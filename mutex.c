#include <pthread.h>
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
#define min_arrival_fuel 35
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
  int fuel;
  int delay;
  bool inAirspace;
  bool havingEmergency;
  bool critical;
}Plane;

// Function called in individual plane threads
void plane_flying(int* ptrToID);

// Mutex related variables
pthread_mutex_t runway_mutex[runways];
pthread_mutex_t something_open;
pthread_cond_t runwayOpen;

// Plane management variables
bool runwayOccupied[runways];
int runwaysFree = runways;
Plane planes[numplanes];
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
  
  //Establish mutexes for each runway
  pthread_t threads[numplanes];
  for(i = 0; i < runways; i++){
    runwayOccupied[i] = FALSE;
    pthread_mutex_unlock(&runway_mutex[i]);
  }
  
  //Create each plane & corresponding thread
  for(i = 0; i < numplanes; i++){
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
      havingEmergency[i] = FALSE;
    }
  }
  
  //Indicate that at least one runway is open
  pthread_mutex_unlock(&something_open);
  
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
            num_critical_planes++;
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
      if(num_critical_planes == 0 || planes[id].critical || planes[id].havingEmergency){ // ^^ *(emergency->fuel)
        for(i = 0;(i < runways) && (finished == 0); i++){ //Cycle through runways
          if(pthread_mutex_trylock(&runway_mutex[i]) == 0) {  
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
            
            //Unlok mutexes
            pthread_mutex_unlock(&runway_mutex[i]);//release the mutex
            pthread_cond_broadcast(&runwayOpen);
          }
        }
        //none of the runways were open
        pthread_cond_wait(&runwayOpen,&something_open);
        //sleep for some time
        pthread_mutex_unlock(&something_open);
      }
    }
  }
  pthread_exit(0);
}
