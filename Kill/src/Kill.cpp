#include <iostream>
#include <err.h>

int main(int argc, char * argv[]) {
	if(argc != 4)
	{
		std::cout << argv[0] << " [SIG] [DATA] [PID]" << std::endl;
		return -1;
	}

	int signalNum = atoi(argv[1]);

	union sigval sigval;
	pid_t pid;

	sigval.sival_int = atoi(argv[2]);
	pid = atoi(argv[3]);

	if(signalNum < SIGRTMIN || signalNum > SIGRTMAX)
	{
		std::cerr << "[SIG] must be from " << SIGRTMIN << " to: " << SIGRTMAX << std::endl;
		return -1;
	}

	/**
	 * Skolejkowanie sygnału, który ma być przekazany do procesu.
	 */
	if(sigqueue(pid, signalNum, sigval) == -1)
	{
		return -1;
	}

	return EXIT_SUCCESS;
}
