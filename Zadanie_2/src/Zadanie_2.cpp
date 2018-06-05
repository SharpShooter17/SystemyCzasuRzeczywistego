#include <iostream>

/**
 * unmask
 * mmap
 * munmap
 * shm_unlinlk
 */
#include <sys/mman.h>

/**
 * shm_open
 */
#include <fcntl.h>


/**
 * ChannelCreate
 * ConnectAttach
 * ConnectDetach
 * MsgSend
 * MsgReceive
 * MsgReply
 */
#include <sys/neutrino.h>

typedef int VECTYPE;
const int VECLEN = 8;
const unsigned int VECBYTES = (sizeof(VECTYPE) * VECLEN);

const char * MSGNAME = "SHMEMQNX";

static int compare(const void * pa, const void * pb)
{
	int a, b;

	a = *((const int * const ) (pa));
	b = *((const int * const ) (pb));

	return b - a;
}

static void child(pid_t spid, int ch)
{
	int conn = ConnectAttach(0, spid, ch, 0, 0);
	if (conn == -1)
	{
		std::cerr << "Connect attach fail" << std::endl;
		return;
	}

	int shm = shm_open(MSGNAME, O_RDWR | O_CREAT | O_TRUNC,
			umask(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
	if (shm == -1)
	{
		std::cerr << "shm_open fail" << std::endl;
		return;
	}

	if (ftruncate(shm, VECBYTES) == -1)
	{
		std::cerr << "fturncate fail" << std::endl;
		return;
	}

	void* memory = mmap(0, VECBYTES, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	if (memory == MAP_FAILED)
	{
		std::cerr << "nmap fail" << std::endl;
		return;
	}

	std::cout << "Generating: " << std::endl;

	for (int i = 0; i < VECLEN; i++)
	{
		int r = rand();

		((int*) (memory))[i] = r;
		std::cout << r << ", ";
	}

	std::cout << std::endl;

	if (MsgSend(conn, MSGNAME, sizeof(MSGNAME), 0, 0) == -1)
	{
		std::cerr << "MsgSend fail" << std::endl;
		return;
	}
	std::cout << "Modify:" << std::endl;
	for (int i = 0; i < VECLEN; i++)
	{
		std::cout<< "- " << ((int*) (memory))[i] << std::endl;
	}

	std::cout << std::endl;

	if (munmap(memory, VECBYTES) == -1)
	{
		std::cerr << "munmap fail" << std::endl;
		return;
	}

	if ( close(shm) == -1)
	{
		std::cerr << "Close fail" << std::endl;
	}

	if (shm_unlink(MSGNAME) == -1)
	{
		std::cerr << "shm_unlink fail" << std::endl;
	}

	if (ConnectDetach(conn) == -1)
	{
		std::cerr << "Connect detach fail" << std::endl;
		return;
	}
}

static void parent(pid_t cpid, int ch)
{
	char msgname[sizeof(MSGNAME)];

	int recv = MsgReceive(ch, msgname, sizeof(MSGNAME), 0);

	if (recv == -1)
	{
		std::cerr << "MsgReceive fail" << std::endl;
		return;
	}

	int shm = shm_open(msgname, O_RDWR,
			umask(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
	if (shm == -1)
	{
		std::cerr << "shm_open fail" << std::endl;
		return;
	}

	void * memory = mmap(0, VECBYTES, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	if (memory == MAP_FAILED)
	{
		std::cerr << "nmap fail" << std::endl;
		return;
	}

	qsort(memory, VECLEN, sizeof(VECTYPE), compare);

	if (MsgReply(recv, 0, msgname, sizeof(MSGNAME)) == -1)
	{
		std::cerr << "MsgReply fail" << std::endl;
		return;
	}

	if (munmap(memory, VECBYTES) == -1)
	{
		std::cerr << "munmap fail" << std::endl;
		return;
	}

	if (close(shm) == -1)
	{
		std::cerr << "close fail" << std::endl;
		return;
	}
}

int main(int argc, char * argv[])
{
	int ch = ChannelCreate(0);

	if (ch == -1)
	{
		std::cerr << "Channel create fail" << std::endl;
		return -1;
	}

	pid_t pid = fork();

	if (pid == -1)
	{
		std::cerr << "Fork fail" << std::endl;
		return -1;
	}

	if (pid == 0)
	{
		child(getppid(), ch);
	} else {
		parent(pid, ch);
	}

	return EXIT_SUCCESS;
}
