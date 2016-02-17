#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
  
typedef int bool;
#define TRUE 1
#define FALSE 0
  
#define runways 3
#define numplanes 25
#define chance_for_havingEmergency 8
#define max_fuel 100
#define min_arrival_fuel 45
#define critical_fuel 20
#define taxi_time 3
#define max_descend_time 10
#define min_descend_time 4
 
/* Struct: Plane
 * Vars:
 *   fuel: The amount of fuel left in the plane
 *   delay: How much time is left until the plane arrives
 *   inAirspace: Is the plane in the local airspace
 *   havingEmergency: Does the plane require an emergency landing
 *   critical: Does the plane have a critical fuel level
 */
typedef struct Plane{
  int id;
  int fuel;
  int delay;
  bool inAirspace;
  bool havingEmergency;
  bool critical;
}Plane;

void sortFuel(void);

// Function called in individual plane threads
void plane_flying(int* ptrToID);

// Semaphore related variables
sem_t runway_semaphore[runways];
sem_t something_open;

// Plane management variables
bool runwayOccupied[runways];
int runwaysFree = runways;
Plane planes[numplanes];
Plane lowFuelPriority[numplanes];
int nextPriority = 0;
int num_emergencies = 0;
int num_critical_planes = 0;
int planes_landed = 0;

// Main
int main(){
  int i = 0;
  time_t t;
  
  //Create random seed
  srand((unsigned) time(&t));
  printf("Using seed %u.\n",(unsigned int)t);
  
  //Establish Semaphores for each runway
  pthread_t threads[numplanes];
  for(i = 0; i < runways; i++){
    runwayOccupied[i] = FALSE;
    sem_init(&runway_semaphore[i],0,1);
  }
  
  //Create each plane & corresponding thread
  for(i = 0; i < numplanes; i++){
    planes[i].id = i;
    planes[i].delay = i * (1 + rand()%3);
    planes[i].fuel = min_arrival_fuel + (rand() % (max_fuel - min_arrival_fuel));
    int* id = malloc(sizeof(int));
    *id = i;
    pthread_create (&(threads[i]),NULL, (void *) &plane_flying, id);
    planes[i].critical = FALSE;
    if (rand() % chance_for_havingEmergency == 0){
      planes[i].havingEmergency = TRUE;
    }
    else{
      planes[i].havingEmergency = FALSE;
    }
    lowFuelPriority[i] = planes[i];
  }

//sort the planes by fuel level
  sortFuel();

  
  //initialize that one runway is open
  sem_init(&something_open,0,1);
  
  //First plane always arrives at 0s, must be manually initialized
  planes[0].inAirspace = TRUE;
  
  //This loop acts as a timer and handles fuel management & plane arrivals
  while(planes_landed < numplanes){
    sleep(1);
    for(i = 0; i < numplanes; i++){
      if(planes[i].delay > 0){ //If plane hasn't arrived yet
        planes[i].delay--;
        if(planes[i].delay == 0) 
          planes[i].inAirspace = TRUE;
      }
      else{ //plane already in airspace
        planes[i].fuel--;
        if(planes[i].fuel == 0 && planes[i].inAirspace){
          printf("Plane # %d has crashed.\n",i);
          return -1;
        }
        else if(planes[i].fuel < critical_fuel && planes[i].inAirspace){
          if(planes[i].critical == FALSE){
            planes[i].critical = TRUE;
            printf("Plane # %d is low on fuel!\n",i);
          }
        }
      }
    }
  }
  
  printf("All planes have landed successfully.\n");
  return 0;
}

// Function run in individual plane threads
void plane_flying(int* ptrToID){
  int id = *ptrToID;
  int i = 0;
  
  while(!planes[id].inAirspace) //Do nothing until plane arrives
	  sleep(1);
  printf("Plane # %d has arrived.\n",id);
  
  if(planes[id].havingEmergency){
    num_emergencies++;
    printf("Plane # %d is having an emergency.\n",id);
  }
  int finished = FALSE;
  int lowest_fuel = max_fuel;
  
  //Idle until available runway
  while(finished == 0){
    if(num_emergencies == 0 || planes[id].havingEmergency){ //If no other planes are having an emergency OR this plane is having an emergency
      if( ( lowFuelPriority[nextPriority].inAirspace == TRUE && lowFuelPriority[nextPriority].fuel == planes[id].fuel ) || planes[id].havingEmergency || lowFuelPriority[nextPriority].inAirspace == FALSE){ // ^^ *(emergency->fuel)
        for(i = 0;(i < runways) && (finished == 0); i++){ //Cycle through runways
          if(sem_trywait(&runway_semaphore[i]) == 0) {
			//Plane is landing
            planes[id].inAirspace = FALSE;
            
            //Handle emergency vars
            if(planes[id].havingEmergency){
              num_emergencies--; 
              planes[id].havingEmergency = FALSE;
            }
            //Handle critical vars
            if(planes[id].critical){
              num_critical_planes--;
	      nextPriority++;
              planes[id].critical = FALSE;
            }
            
            //Manage descent
            printf("Plane # %d is starting to descend on runway %d.\n",id,(i+1));
            sleep((rand() % max_descend_time) + min_descend_time);//it takes anywhere between 10 and 30 seconds to land
            printf("Plane # %d has landed and is now clearing from runway %d.\n",id,(i+1));
            sleep(taxi_time);
            printf("Plane # %d has cleared from runway %d.\n",id,(i+1));
            planes_landed++;
            finished = TRUE;
            
            //Up the semaphore
            sem_post(&runway_semaphore[i]);
	    sem_post(&something_open);
	    
          }
        }
        //none of the runways were open
        sem_wait(&something_open);
      }
    }
  }
  pthread_exit(0);
}

void sortFuel(void){






}
