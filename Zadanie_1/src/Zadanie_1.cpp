#include <iostream>

struct SignalParam
{
	int signal;
};

void * thread(void * params)
{
	SignalParam * signalParams = reinterpret_cast<SignalParam*>(params);
	int numOfSignal = signalParams->signal;
	delete signalParams;

	sigset_t sigset;
	siginfo_t siginfo;

	sigfillset(&sigset);
	sigdelset(&sigset, numOfSignal);

	pthread_sigmask(SIG_SETMASK, &sigset, NULL);

	sigemptyset(&sigset);
	sigaddset(&sigset, numOfSignal);

	while (true)
	{
		int currentSignal = sigwaitinfo(&sigset, &siginfo);
		std::cout << "Signal number: " << currentSignal << " data: " <<  siginfo.si_value.sival_ptr << std::endl;
	}

	return NULL;
}

void theEnd( const std::string & info)
{
	std::cerr << "Error: " << info << std::endl;
	abort();
}

int main()
{
	const int size = SIGRTMAX - SIGRTMIN + 1;
	pthread_t threads[size];
	sigset_t sigset;

	sigemptyset(&sigset);

	for (int id = 0; id < size; id++)
	{
		int signalNumber = id + SIGRTMIN;
		sigaddset(&sigset, signalNumber);
		SignalParam * params = new SignalParam();
		params->signal = signalNumber;
		int result = pthread_create(&threads[id], NULL, thread, params);

		if (result != EOK)
		{
			theEnd("Loop: pthread_create");
		}
	}

	pthread_sigmask(SIG_BLOCK, &sigset, NULL);

	for ( int id = 0; id < size; id++) {
		int result = pthread_join(threads[id], NULL);
		if (result != EOK)
		{
			theEnd("Loop: pthread_join");
		}
	}

	return 0;
}
