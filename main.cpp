#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <semaphore.h>
#include <cstdlib>   // srand() and rand()
#include <ctime>     // 
#include <unistd.h>  // 
#include <chrono>    // For measuring time durations

using namespace std;
using namespace std::chrono;

const int MAX_CHAIRS = 3; // Max chairs in the waiting room
int waitingCustomers = 0; // Number of waiting customers
sem_t customers;          // Semaphore for waiting customers
sem_t barberSem;          // Semaphore for the barber
mutex mtx;                // Mutex for chairs

// Vector clocks
vector<int> barberClock(2, 0);   // Barber's vector clock (2 processes: barbr, customer)
vector<vector<int>> customerClocks(10, vector<int>(2, 0)); // Customer vector clock

// Function for print the vector clock
void printClock(const vector<int>& clock, const string& name) {
    cout << name << " clock: [";
    for (size_t i = 0; i < clock.size(); ++i) {
        cout << clock[i];
        if (i < clock.size() - 1)
            cout << ", ";
    }
    cout << "]" << endl;
}

// Update vector clock 
void updateClock(vector<int>& receiverClock, const vector<int>& senderClock) {
    for (size_t i = 0; i < receiverClock.size(); ++i) {
        receiverClock[i] = max(receiverClock[i], senderClock[i]);
    }
}

// Barber thread function
void barberThread() {
    while (true) {
        // Wait for a customer
        sem_wait(&customers);

        // Record the start time of the haircut
        auto startTime = high_resolution_clock::now();

        // Protect shared resources with mutex
        mtx.lock();
        waitingCustomers--;
        cout << "Barber is cutting hair. Waiting customers number: " << waitingCustomers << endl;

        // Update the barber's vector clock
        barberClock[0]++;  // Increment barber clock
        printClock(barberClock, "Barber");

        mtx.unlock();

        // Simulate hair cutting using rand() 
        sleep(rand() % 5 + 2);  // Sleep for 1 to 3 seconds

        // Record the end time of the haircut then calculate the duration
        auto endTime = high_resolution_clock::now();
        auto duration = duration_cast<seconds>(endTime - startTime);
        cout << "Barber finished cutting hair in " << duration.count() << " seconds." << endl;

        // Signal the barber is ready to cut the next customer hair
        sem_post(&barberSem);
    }
}

// Customer thread function
void customerThread(int id) {
    mtx.lock();
    if (waitingCustomers < MAX_CHAIRS) {
        waitingCustomers++;
        cout << "Customer c" << id << " is waiting. Waiting customers: " << waitingCustomers << endl;

        // Update the customer's vector clock for the event
        customerClocks[id][1]++;  // Increment customer clock
        printClock(customerClocks[id], "Customer c" + to_string(id));

        mtx.unlock();

        // Record the time the customer starts waiting
        auto waitStartTime = high_resolution_clock::now();

        // signal to barber that a customer is waiting
        sem_post(&customers);

        // Wait for the barber to be ready
        sem_wait(&barberSem);

        // Record the time the customer starts the haircut
        auto waitEndTime = high_resolution_clock::now();
        auto waitDuration = duration_cast<seconds>(waitEndTime - waitStartTime);
        cout << "Customer c" << id << " waited for " << waitDuration.count() << " seconds before getting a haircut." << endl;

        // Update clocks after the haircut
        mtx.lock();
        updateClock(barberClock, customerClocks[id]);  // Barber updates
        updateClock(customerClocks[id], barberClock);  // Customer updates
        printClock(barberClock, "Barber");
        printClock(customerClocks[id], "Customer c" + to_string(id));
        mtx.unlock();

        cout << "Customer c" << id << " is getting a haircut." << endl;

    } else {
        cout << "Customer c" << id << " leaves because there are no free chairs." << endl;
        mtx.unlock();
    }
}

int main() {
    
    srand(time(NULL));

    // Initialize semaphores
    sem_init(&customers, 0, 0);  
    sem_init(&barberSem, 0, 0);  

    // Create barber thread
    thread barber(barberThread);

    // Create customer threads
    thread customerThreads[10];
    for (int i = 0; i < 10; i++) {
        sleep(rand() % 2 + 1);  // Random arrival times (1 to 2 seconds)
        customerThreads[i] = thread(customerThread, i);
    }

    // Join customer threads
    for (int i = 0; i < 5; i++) {
        customerThreads[i].join();
    }

    // Destroy semaphores
    sem_destroy(&customers);
    sem_destroy(&barberSem);

    
    barber.detach();  //barber thread run indefinitely

    return 0;
}
