/**
 * A simple class to easily manage a list of tasks
 *
 * You have a global instance of the class : TaskList
 *
 * So, in the end of the setup, you just need to have for example :
 *
 * TaskList.AddTask({true, "Task_Name", 1024, 5, 2000, 5000, CoreAny, Task_Function});
 * TaskList.Create();
 *
 * See TaskData_t struct for the meaning of each data parameters of the task
 * That's all :)
 *
 * The code of the function of the task look like that :
 *
 void Task_Function(void *parameter)
 {
	 TaskData_t *td = TaskList.GetTaskByName("Task_Name");
	 int sleep = pdMS_TO_TICKS(td->Sleep_ms);
	 for (EVER)
	 {
		 ... code
#if RUN_TASK_MEMORY == true
		 td->Memory = uxTaskGetStackHighWaterMark(NULL);
#endif
		 vTaskDelay(sleep);
	 }
 }
 *
 * RUN_TASK_MEMORY is a define to create a special task to know the remaining memory of the tasks (for debug purpose)
 */
#pragma once
#ifndef TASKS_UTILS
#define TASKS_UTILS

#include <Arduino.h>
#include <vector>

// To see the remaining memory of each task. Use it for debug
#ifndef RUN_TASK_MEMORY
#define RUN_TASK_MEMORY	false
#endif

// For infinite loop : for (EVER)
#define EVER ;;

typedef enum
{
	Core0 = 0,
	Core1 = 1,
	CoreAny
} Task_Core;

/**
 * Task structure
 * All parameters are required except Param and Handle
 * Handle should be always initialized to NULL because it will be filled when task is created
 */
typedef struct
{
	public:
		bool Condition;													 // Boolean to determine if we create the task or no
		char Name[configMAX_TASK_NAME_LEN + 1];  // Task name
		configSTACK_DEPTH_TYPE StackSize;        // Task size
		UBaseType_t Priority;                    // Task priority : 0 lower to higher configMAX_PRIORITIES - 1
		int Sleep_ms;                            // Task sleep delay in ms
		int Memory;                              // Task staying memory
		Task_Core Core;													 // Core where is running the task
		TaskFunction_t TaskCode;								 // The code of the task
		void *Param = NULL;											 // The parameter passed to the code function
		TaskHandle_t Handle = NULL;							 // The handle of the task
} TaskData_t;

/**
 * The task list class
 * Set RUN_TASK_MEMORY to true to have Memory task running
 * A global instance of TaskList_c is automaticaly created named TaskList
 */
class TaskList_c
{
	public:
		TaskList_c(bool with_memory = false);

		bool Create(bool with_idle_task);
		void AddTask(const TaskData_t task);

		void DeleteTask(const String &name);
		bool CreateTask(const String &name, bool force = false, void *param = NULL);
		void ChangeSleepTask(const String &name, int sleep_ms, bool reload = false);
		bool IsTaskRunning(const String &name);

		TaskData_t* GetTaskByName(const String &name);
		int GetTaskSleep(const String &name, bool to_ticks = true);

		String GetIdleStr(void) const;
		String GetMemoryStr(void) const;

		// Put in public to have access with the global instance
		void Task_Memory_code(void *parameter);

	protected:
		std::vector<TaskData_t> Tasks;
		TaskData_t TaskMemory;
		bool task_created;
};

/**
 * A global instance of TaskList
 */
#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_TASKLIST)
extern TaskList_c TaskList;
#endif

#endif
