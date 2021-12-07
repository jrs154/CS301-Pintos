#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"

unsigned int v_on_bridge = 0;
const unsigned int MAX_v_on_bridge = 3;
unsigned int num_waiting_to_go_left = 0;
unsigned int num_waiting_to_go_right = 0;
unsigned int vehicles_left;
const int Dir_left = 0;
const int Dir_right = 1;
const int Emer = 0;
const int Norm = 1;
int current_direction = 0;

struct semaphore bridge_lock;
struct semaphore allow_priority;
struct semaphore entry_left_allowed;
struct semaphore entry_right_allowed;

struct list emergency_vehicles;
struct priority_vehicle
{
    struct thread *thread;
    int direction;
    struct list_elem elem;
};
struct vehicle_thread
{
    int direction;
    int priority;
};
int change_dir (int direction)
{
  return (direction == Dir_left) ? Dir_right : Dir_left;
}

void narrow_bridge (unsigned int num_vehicles_left, unsigned int num_vehicles_right,
                    unsigned int num_Emer_left, unsigned int num_Emer_right);


void test_narrow_bridge(void)
{
    narrow_bridge(0, 0, 0, 0);
    narrow_bridge(1, 0, 0, 0);
    narrow_bridge(0, 0, 0, 1);
    narrow_bridge(0, 4, 0, 0);
    narrow_bridge(0, 0, 4, 0);
    narrow_bridge(3, 3, 3, 3);
    narrow_bridge(4, 3, 4 ,3);
    narrow_bridge(7, 23, 17, 1);
    narrow_bridge(40, 30, 0, 0);
    narrow_bridge(30, 40, 0, 0);
    narrow_bridge(23, 23, 1, 11);
    narrow_bridge(22, 22, 10, 10);
    narrow_bridge(0, 0, 11, 12);
    narrow_bridge(0, 10, 0, 10);
    narrow_bridge(0, 10, 10, 0);
    pass();
}

void OneVehicle (int direction, int priority)
{
  ArriveBridge (direction, priority);
  CrossBridge (direction, priority);
  //ExitBridge (direction, priority);
}

void vehicle_func (void *arg)
{
  //printf("enter");
  ASSERT (arg);
  struct vehicle_thread *a = (struct vehicle_thread *) arg;
  OneVehicle (a->direction, a->priority);
  free (arg);
  //printf("exit");
}

void make_threads (unsigned int num_vehicles, int direction, int priority, tid_t *threads, int *num_thread)
{
  for (int i = 0; i < num_vehicles; i++)
    {
      struct vehicle_thread *arg = malloc (sizeof (struct vehicle_thread));
      arg->priority = priority;
      arg->direction = direction;
      threads[(*num_thread)++] = thread_create ("", PRI_DEFAULT, vehicle_func, arg);
    }
}


void ArriveBridge (int direction, int priority)
{
  struct semaphore *allow_entry;
  ASSERT((direction == Dir_left || direction == Dir_right) && (priority == Emer || priority == Norm));
  if (priority == Emer)
    allow_entry = &allow_priority;
  else if (direction == Dir_left)
    allow_entry = &entry_left_allowed;
  else
    allow_entry = &entry_right_allowed;

  sema_down (&bridge_lock);

  if (list_empty (&emergency_vehicles) && (v_on_bridge == 0 || (v_on_bridge < MAX_v_on_bridge && current_direction == direction)))
  {
      current_direction = direction;
      v_on_bridge++;
      sema_up (allow_entry);
  }
  else
    {
      if (priority == Emer)
      {
          struct priority_vehicle v = 
          {
          .direction = direction,
          .thread = thread_current ()
          };
          list_push_back (&emergency_vehicles, &v.elem);
      }
      else
      {
          if (direction == Dir_left)
            num_waiting_to_go_left++;
          else
            num_waiting_to_go_right++;
      }
    }

  sema_up (&bridge_lock);
  sema_down (allow_entry);
}

void CrossBridge (int direction, int priority)
{
  uint8_t t;
  random_bytes(&t, sizeof(t));
  int64_t random_time_amount = t;
  printf ("The vehicle is entering to %s as %s\n", direction == Dir_left ? "left" : "right", priority == Emer ? "Emergency" : "Normal");
  //printf ("Vehicle takes %"PRId64"\n", random_time_amount);
  timer_sleep (random_time_amount);
  printf ("Vehicle exits to %s as %s\n", direction == Dir_left ? "left" : "right", priority == Emer ? "Emergency" : "Normal");  
}

void narrow_bridge (unsigned int num_vehicles_left, unsigned int num_vehicles_right,
                    unsigned int num_Emer_left, unsigned int num_Emer_right)
{
  list_init (&emergency_vehicles);
  sema_init (&entry_left_allowed, 0);
  sema_init (&bridge_lock, 1);
  sema_init (&entry_right_allowed, 0);
  sema_init (&allow_priority, 0);
  

  vehicles_left = num_vehicles_left + num_vehicles_right + num_Emer_left + num_Emer_right;
  tid_t threads[vehicles_left];
  int num_thread = 0;

  make_threads (num_Emer_left, Dir_left, Emer, threads, &num_thread);
  make_threads (num_Emer_right, Dir_right, Emer, threads, &num_thread);
  make_threads (num_vehicles_right, Dir_right, Norm, threads, &num_thread);
  make_threads (num_vehicles_left, Dir_left, Norm, threads, &num_thread);

  for (int i = 0; i < num_thread; i++)
    {
      printf("thread %d has id (%d)\n", i, threads[i]);
      //timer_sleep(100);
    }
}

