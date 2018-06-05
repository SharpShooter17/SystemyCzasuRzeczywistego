#include <iostream>
#include <time.h>
#include <memory.h>
#include <algorithm>
/*
 * define THREAD_POOL_PARAM_T such that we can avoid a compiler
 * warning when we use the dispatch_*() functions below
 */
#define THREAD_POOL_PARAM_T dispatch_context_t

/**
 * iofunc_attr_t
 * iofunc_read_verify
 * iofunc_func_init
 * iofunc_attr_init
 */
#include <sys/iofunc.h>
/**
 * thread_pool_attr_t
 * thread_pool_create
 * thread_pool_t
 */
#include <sys/dispatch.h>

static resmgr_connect_funcs_t    connect_funcs;
static resmgr_io_funcs_t         io_funcs;
static iofunc_attr_t             attr;

#define BUFSIZE 128
static char buffer[BUFSIZE];


int io_read (resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb)
{
    time_t t;
    struct tm td;

    time(&t);
    localtime_r(&t, &td);
    strftime(buffer, BUFSIZE, "%c\n", &td);
    attr.nbytes = strlen(buffer) + 1;

    int status;
    if ((status = iofunc_read_verify (ctp, msg, ocb, NULL)) != EOK)
        return (status);

    if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE)
        return (ENOSYS);

    /*
     *  On all reads (first and subsequent), calculate
     *  how many bytes we can return to the client,
     *  based upon the number of bytes available (nleft)
     *  and the client's buffer size
     */

    int nleft = ocb->attr->nbytes - ocb->offset;
    int nbytes = std::min (msg->i.nbytes, static_cast<_Uint32t>( nleft ));

    int nparts;

    if (nbytes > 0)
    {
        /* set up the return data IOV */
        SETIOV (ctp->iov, buffer + ocb->offset, nbytes);

        /* set up the number of bytes (returned by client's read()) */
        _IO_SET_READ_NBYTES (ctp, nbytes);

        /*
         * advance the offset by the number of bytes
         * returned to the client.
         */

        ocb->offset += nbytes;

        nparts = 1;
    } else
    {
        /*
         * they've asked for zero bytes or they've already previously
         * read everything
         */

        _IO_SET_READ_NBYTES (ctp, 0);

        nparts = 0;
    }

    /* mark the access time as invalid (we just accessed it) */

    if (msg->i.nbytes > 0)
    {
        ocb->attr->flags |= IOFUNC_ATTR_ATIME;
    }

    return (_RESMGR_NPARTS (nparts));
}

int io_write (resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb)
{
    int status;
    if ((status = iofunc_write_verify(ctp, msg, ocb, NULL)) != EOK)
        return (status);

    if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE)
    {
        return(ENOSYS);
    }

    /* set up the number of bytes (returned by client's write()) */

    _IO_SET_WRITE_NBYTES (ctp, msg->i.nbytes);

    char* buf = (char *) malloc(msg->i.nbytes + 1);
    if (buf == NULL)
    {
        return(ENOMEM);
    }

    /*
     *  Reread the data from the sender's message buffer.
     *  We're not assuming that all of the data fit into the
     *  resource manager library's receive buffer.
     */

    resmgr_msgread(ctp, buf, msg->i.nbytes, sizeof(msg->i));
    buf [msg->i.nbytes] = '\0'; /* just in case the text is not NULL terminated */
    //printf ("Received %d bytes = '%s'\n", msg -> i.nbytes, buf);
    free(buf);

    int i = atoi(buf);
    struct timespec t;
    t.tv_sec = i;
    t.tv_nsec = 0;

    clock_settime(CLOCK_REALTIME, &t);

    if (msg->i.nbytes > 0)
    {
        ocb->attr->flags |= IOFUNC_ATTR_MTIME | IOFUNC_ATTR_CTIME;
    }

    return (_RESMGR_NPARTS (0));
}


int main(int argc, char* argv[])
{
    /* initialize dispatch interface */
    dispatch_t * dpp = dispatch_create();
    if(dpp == NULL)
    {
    	std::cerr << "Allocate dipatch fail" << std::endl;
        return -1;
    }

    /* initialize resource manager attributes */
    resmgr_attr_t resmgr_attr;
    memset(&resmgr_attr, 0, sizeof resmgr_attr);
    resmgr_attr.nparts_max = 1;
    resmgr_attr.msg_max_size = 2048;

    /* initialize functions for handling messages */
    iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs, _RESMGR_IO_NFUNCS, &io_funcs);
    io_funcs.read = io_read;
    io_funcs.write = io_write;

    /* initialize attribute structure used by the device */
    iofunc_attr_init(&attr, S_IFNAM | 0666, 0, 0);

    /* attach our device name */
    int id = resmgr_attach(dpp,            /* dispatch handle        */
                       &resmgr_attr,   /* resource manager attrs */
                       "/dev/scr",     /* device name            */
                       _FTYPE_ANY,     /* open type              */
                       0,              /* flags                  */
                       &connect_funcs, /* connect routines       */
                       &io_funcs,      /* I/O routines           */
                       &attr);         /* handle                 */
    if(id == -1)
    {
    	std::cerr << "Attach name fail" << std::endl;
        return -1;
    }

    /* initialize thread pool attributes */
    thread_pool_attr_t pool_attr;
    memset(&pool_attr, 0, sizeof pool_attr);
    pool_attr.handle = dpp;
    pool_attr.context_alloc = dispatch_context_alloc;
    pool_attr.block_func = dispatch_block;
    pool_attr.unblock_func = dispatch_unblock;
    pool_attr.handler_func = dispatch_handler;
    pool_attr.context_free = dispatch_context_free;
    pool_attr.lo_water = 2;
    pool_attr.hi_water = 4;
    pool_attr.increment = 1;
    pool_attr.maximum = 50;

    /* allocate a thread pool handle */
    thread_pool_t* tpp = thread_pool_create(&pool_attr, POOL_FLAG_EXIT_SELF);
    if(tpp == NULL)
    {
    	std::cerr << "Initialize thread pool fail" << std::endl;
        return -1;
    }

    /* start the threads, will not return */
    thread_pool_start(tpp);

    return EXIT_FAILURE;
}
