#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int main(void)
{
	puts("Hello World!!!");
	printf( "This is thread %d\n", pthread_self() );
	return EXIT_SUCCESS;
}

/******************************************************************************************************
 *  Napisać program obsługujący sygnały czasu rzeczywistego.
 * Jeden pies co będzie robił.
 * Ma spełniać:
 * - obsługiwane dwa różne sygnały czasu rzeczywistego.
 * - obsługa sygnałów ma być synchroniczna. Dedykowany wątek dla zadania. Jeden sygnał jeden wątek.
 * - program umie odebrać dane wysyłane z sygnału.
 * - kill nie umozliwia dołączenia danych do sygnału.
 * - drugi program będący odpowiednikiem programu kill plus wysyłanie danych.
 *
 * Przydatne funkcje:
 * - pthread_sigmask - dodaje maske do watku sygnalu
 * - sigfillset - wypełnia zbior
 * - sigemptyset - czysci zbior
 * - sigaddset - dodaje pojedynczy sygnal do zbioru
 * - sigdelset - usuwa pojedynczy sygnal ze zbioru
 * - sigwaitinfo - funkcja do odbierania sygnalu - watek jest blokowany do momentu nadejscia sygnalu
 * - sigqueue - funkcja umożliwiająca wysłanie sygnału z danymi
 *
 * SPECIAL SIGNALS - HELP
 * ****************************************************************************************************/

