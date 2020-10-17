//
// Created by omermosa on 5/5/20.
// all rights reserved
//
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <math.h>

#define  lp(i,n,s) for(int i=s;i<n;i++)

#define  bathroom_capacity 4 // max capacity of the bathroom

long int total_men_in=0, total_men_out=0, total_women_in=0, total_women_out=0;// total num of men/ women who entered , finished.

long int num_women_arrived=0, num_men_arrived=0;

long int num_men=0,num_women=0; //num of men / women present in bathroom

pthread_mutex_t mutex, wm,mm; //for mutual execution
pthread_cond_t not_allowed; //not allowed  either not same gender in bathroom or bathroom is full capacity.

sem_t Q; // semaphore Queue to enforce a waiting order and guarantee that the bathroom doesn't have to be full with one gender before it allows the next.

void  woman_wants_to_enter()
{
    sem_wait(&Q); //take a place in the bathroom or  join the waiting queue if no place in bathroom
    pthread_mutex_lock(&mutex); //obtain a lock (mutex) to execute the critical section

    while(num_women >= bathroom_capacity || num_men !=0) // mark as critical section / don't enter if capacity exceeded or there is another gender inside
        pthread_cond_wait(&not_allowed, &mutex);

    num_women++; // increment the num of women in the bathroom
    total_women_in++;// inc the total num of women who have been served so far
    printf("     Woman num %ld Enters, num women in the bathroom : %ld \n", total_women_in, num_women); // print the women ID and the current women in bathrrom to make things clear.
    pthread_mutex_unlock(&mutex); //release the lock.
    sem_post(&Q); // leave the bathroom or  waiting queue.

}


void  man_wants_to_enter() //same logic as women wants to enter function
{
    sem_wait(&Q); //same as women wants to enter func
    pthread_mutex_lock(&mutex);
    while(num_men >= bathroom_capacity || num_women !=0)
        pthread_cond_wait(&not_allowed, &mutex);
    num_men++;
    total_men_in++;
    printf("     Man num %ld  Enters, num men in the bathroom :  %ld \n", total_men_in, num_men);

    pthread_mutex_unlock(&mutex);
    sem_post(&Q);// same as women wwants to enter func

}

void  woman_leaves()
{

    pthread_mutex_lock(&mutex); //acquire the lock
    total_women_out++; //inc the number of women out
    num_women--; //woman finished

    pthread_cond_broadcast(&not_allowed); //signal all threads

    printf("             -->> Woman num %ld left, num women in the bathroom  %ld \n", total_women_out, num_women);

    pthread_mutex_unlock(&mutex); //unlock


}


void  man_leaves() //same logic as women want to leave
{

    pthread_mutex_lock(&mutex);
    total_men_out++;
    num_men--; //man finished
    pthread_cond_broadcast(&not_allowed); //signal all
    printf("           -->> Man num %ld left, num men in the bathroom : %ld\n", total_men_out, num_men);

    pthread_mutex_unlock(&mutex);


}
void* women_function(void*arg) //integrates two functions above together (thread function)
{

    // these following lines are for the purpose of calculating a fairness measure at the end; they have nothing to do with synchornization solution.
        pthread_mutex_lock(&wm);
        num_women_arrived++;
        pthread_mutex_unlock(&wm);

    printf("New Woman arrives\n"); // print arrival to monitor arrival and service
    woman_wants_to_enter(); //enter the bathrrom or request it

    usleep(rand()%40000); // random wait for the thread to simulate that it takes time (from 0 to 40 millseconds)
    woman_leaves();// leave the bathroom

    pthread_exit(0);
}
void* men_function(void*arg) //same logic as women function
{
        pthread_mutex_lock(&mm);
        num_men_arrived++;
        pthread_mutex_unlock(&mm);

    printf("New man arrives\n");
    man_wants_to_enter();
    usleep(rand()%40000);

    man_leaves();

    pthread_exit(0);
}

long int max(long int a ,long int b) //get the max between two numbers
{
    if(a>b) return a;
    else return  b;
}

int main()
{

    srand(time(NULL));

    double  start=clock();//measure time taken by the whole prog.

    long int n,m;
    long int max_num_women_waiting=0, max_num_men_waiting=0,diff;

    //input the num of threads by the user
    printf("Enter the num of men threads: ");
    scanf("%ld", &n);
    printf("Enter the num of women threads: ");
    scanf("%ld", &m);
    printf("\n................................................\n");


    sem_init(&Q,0,max(bathroom_capacity/2,1) );// define the Queue semaphor capacity as bathromcap/2


    pthread_t men_threads[n],women_threads[m];//intialize the threads
    pthread_attr_t attr;

    pthread_attr_init(&attr); //initialize the threads attributes

    // launch threads one after the other to get some fairness
    lp(i,max(n,m),0){

        if (i<m)
            if(pthread_create(&women_threads[i],&attr,women_function,NULL)!=0);  // launch a woman thread

            // these lines are to get the maximum number of women that arrived and had to wait without getting in the bathrrom
            diff=num_women_arrived-total_women_in;
            if (diff > max_num_women_waiting )
                max_num_women_waiting=diff;

        if(i<n )
            if(pthread_create(&men_threads[i],&attr,men_function,NULL)!=0);// launch a man thread


            // these lines are to get the maximum number of men that arrived and had to wait without getting in the bathrrom
            diff=num_men_arrived-total_men_in;
            if (diff > max_num_men_waiting )
                max_num_men_waiting=diff;
    }

    // use join threads on both man and women threads to wait for all threads to finish before proceeding in the code.
    lp(i,max(n,m),0){
        if (i<m)
            pthread_join(women_threads[i],NULL);
        if(i<n)
            pthread_join(men_threads[i],NULL);
    }

    double end = clock();
    double time = (double)(end - start) / CLOCKS_PER_SEC; //measure time taken by the whole prog.
    printf("\n................................................\n");

    // check if scheduled correctly and print out the time, fairness measure.
    if (total_women_out==m && total_men_out==n &&total_men_out==total_men_in &&total_women_out==total_women_in) {
        printf("**** Scheduled Correctly\n");

        printf("Un-Fairness Measure: %f\n", fabs(max_num_men_waiting / (float)n - max_num_women_waiting/(float)m) );

        printf("--- Time Taken: %f \n",time);
    }

    else printf("***** Error somewhere \n"); // if anything go wrong.

    return 0;
}
