#include <iostream>
#include <algorithm>
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

const char * MSGNAME = "SHARED_MEMORY";

static void child(pid_t spid, int ch)
{
	/**
	 * Ustanowienie połączenia między procesem a kanałem komunikacji
	 */
	int conn = ConnectAttach(0, spid, ch, 0, 0);
	if (conn == -1)
	{
		std::cerr << "CHILD: Connect attach fail" << std::endl;
		return;
	}

	/**
	 * shm_open - Otwiera obiekt współdzielony
	 * umask - maska pliku dla procesu
	 */
	int shm = shm_open(MSGNAME, O_RDWR | O_CREAT | O_TRUNC, umask(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
	if (shm == -1)
	{
		std::cerr << "CHILD: shm_open fail" << std::endl;
		return;
	}

	/**
	 * ftruncate - plik ma tylko tyle bajtow
	 */
	if (ftruncate(shm, VECBYTES) == -1)
	{
		std::cerr << "CHILD: fturncate fail" << std::endl;
		return;
	}

	/**
	 * mmap - mapowanie obiektu do przestrzeni adresowej procesu
	 */
	void* memory = mmap(0, VECBYTES, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	if (memory == MAP_FAILED)
	{
		std::cerr << "CHILD: nmap fail" << std::endl;
		return;
	}

	std::cout << "CHILD: Generating: " << std::endl;

	for (int i = 0; i < VECLEN; i++)
	{
		int r = rand();

		((int*) (memory))[i] = r;
		std::cout << r << ", ";
	}

	std::cout << std::endl;

	/**
	 * MsgSend - wysyłanie wiadomości do kanału komunikacji
	 */
	if (MsgSend(conn, MSGNAME, sizeof(MSGNAME), 0, 0) == -1)
	{
		std::cerr << "CHILD: MsgSend fail" << std::endl;
		return;
	}

	std::cout << "CHILD: Modify:" << std::endl;
	for (int i = 0; i < VECLEN; i++)
	{
		std::cout<< ((int*) (memory))[i] << ", ";
	}

	std::cout << std::endl;

	/**
	 * unmap - Zwrócenie zmapowanej przestrzeni adresowej
	 */
	if (munmap(memory, VECBYTES) == -1)
	{
		std::cerr << "CHILD: munmap fail" << std::endl;
		return;
	}

	/**
	 * close - zamykanie współdzielonego obiektu.
	 */
	if ( close(shm) == -1)
	{
		std::cerr << "CHILD: Close fail" << std::endl;
	}

	/**
	 * shm_unlink - usuwanie współdzielonego obiektu
	 */
	if (shm_unlink(MSGNAME) == -1)
	{
		std::cerr << "CHILD: shm_unlink fail" << std::endl;
	}

	/**
	 * ConnectDetach - przerwanie połączenia między procesem a kanałem komunikacji
	 */
	if (ConnectDetach(conn) == -1)
	{
		std::cerr << "CHILD: Connect detach fail" << std::endl;
		return;
	}
}

static void parent(pid_t cpid, int ch)
{
	char msgname[sizeof(MSGNAME)];

	/**
	 * MsgReceive - czeka na wiadomość
	 */
	int recv = MsgReceive(ch, msgname, sizeof(MSGNAME), 0);

	if (recv == -1)
	{
		std::cerr << "PARENT: MsgReceive fail" << std::endl;
		return;
	}

	int shm = shm_open(MSGNAME, O_RDWR,
			umask(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
	if (shm == -1)
	{
		std::cerr << "PARENT: shm_open fail" << std::endl;
		return;
	}

	void * memory = mmap(0, VECBYTES, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	if (memory == MAP_FAILED)
	{
		std::cerr << "PARENT: nmap fail" << std::endl;
		return;
	}

	int * tab = (int*)memory;

	for( int i = 0; i < VECLEN; i++ )
	{
		for( int j = 0; j < VECLEN - 1; j++ )
		{
			if( tab[ j ] > tab[ j + 1 ] )
			{
				std::swap( tab[ j ], tab[ j + 1 ] );
			}
		}
	}

	/**
	 * MsgReply - Zwrocenie odpowiedzi
	 */
	if (MsgReply(recv, 0, msgname, sizeof(MSGNAME)) == -1)
	{
		std::cerr << "PARENT: MsgReply fail" << std::endl;
		return;
	}

	if (munmap(memory, VECBYTES) == -1)
	{
		std::cerr << "PARENT: munmap fail" << std::endl;
		return;
	}

	if (close(shm) == -1)
	{
		std::cerr << "PARENT: close fail" << std::endl;
		return;
	}
}

int main(int argc, char * argv[])
{
	/**
	 * Tworzenie kanału komunikacji - odbiór wiadomości
	 */
	int ch = ChannelCreate(0);

	if (ch == -1)
	{
		std::cerr << "MAIN: Channel create fail" << std::endl;
		return -1;
	}

	pid_t pid = fork();

	if (pid == -1)
	{
		std::cerr << "MAIN: Fork fail" << std::endl;
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
