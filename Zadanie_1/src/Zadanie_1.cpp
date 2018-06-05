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
	/**
	 * Informacja o sygnale
	 */
	siginfo_t siginfo;

	/**
	 * sigfillset - inicjalizuje zbiór aby zawierał wszystkie sygnały
	 */
	sigfillset(&sigset);
	/**
	 * sigdelset - Usuwa ze zbioru jeden sygnal
	 */
	sigdelset(&sigset, numOfSignal);

	/**
	 * pthread_sigmask - który wątek ma być wywoływany dla danego sygnału
	 */
	pthread_sigmask(SIG_SETMASK, &sigset, NULL);

	sigemptyset(&sigset);
	sigaddset(&sigset, numOfSignal);

	while (true)
	{
		/**
		 * Czeka na odpowiedni sygnał i zwraca informacje
		 */
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
	/**
	 * SIGRTMIN - SIGRTMAX - Ilość sygnałów czasu rzeczywistego (41 - 56)
	 */
	const int size = SIGRTMAX - SIGRTMIN + 1;
	pthread_t threads[size];

	/**
	 * 2 x Tablica long int
	 */
	sigset_t sigset;

	/**
	 * Czyszczenie sigset - nie zawiera sygnałów
	 */
	sigemptyset(&sigset);

	for (int id = 0; id < size; id++)
	{
		int signalNumber = id + SIGRTMIN;
		/**
		 * Dodawanie sygnału do zbioru
		 */
		sigaddset(&sigset, signalNumber);
		SignalParam * params = new SignalParam();
		params->signal = signalNumber;
		int result = pthread_create(&threads[id], NULL, thread, params);

		if (result != EOK)
		{
			theEnd("Loop: pthread_create");
		}
	}


	/**
	 * Modyfikuje maske sygnału
	 * SIG_BLOCK - Tworzy unie z maski
	 */
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
