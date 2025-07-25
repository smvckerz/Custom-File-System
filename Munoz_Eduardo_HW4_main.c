/**************************************************************
* Class::  CSC-415-0# Spring 2025
* Name:: Eduardo Munoz
* Student ID:: 922499965
* GitHub-Name::
* Project:: Assignment 4 – Processing FLR Data with Threads
*
* File:: Munoz_Eduardo_HW4_main.c
*
* Description::
*
**************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>


/*
The structure Numbers is used to store the statistics for each metric.
*/
typedef struct
{
	pthread_mutex_t mutex;
	int *value;
	int count;
	int cap;
	long long sum;
	int min;
	double mean;
	double standardDev;
	int max;
	long long quart1;
	long long median;
	long long quart3;
	long lowerBound;
	long upperBound;
	long interQuart;
} Numbers;


/*
The structure eventStats is used to store the statistics for each event type.
*/
typedef struct
{
	char *name;
	Numbers dispatch;
	Numbers enroute;
	Numbers overall;
} eventStats;

typedef struct
{
	int openFile;
	long recordIndex;
	long recordCount;
} threadCount;

/*
The enum Metric is used to define the different metrics that can be calculated.
*/
typedef enum
{
	metricDispatch,
	metricEnroute,
	metricOverall
} Metric;
/*
Global variabes used to store the header information, mutex for thread safety,
and the lists of event statistics for all entries and sub areas.
*/
pthread_mutex_t listMutex = PTHREAD_MUTEX_INITIALIZER;
int numFields = 0;
int width[40];
int offset[40];
char *headerName[40];
int recordLength = 0;
char *subArea1 = NULL;
char *subArea2 = NULL;
int AllEntries = 0;
int AllSubArea1 = 0;
int AllSubArea2 = 0;
int capacityOfAll = 0;
int capacityOfSub1 = 0;
int capacityOfSub2 = 0;
int ReceivedID = -1;
int DispatchID = -1;
int EnrouteID = -1;
int onsceneID = -1;
int indexHeader = -1;
int FinalCallID = -1;
int OriginalCallID = -1;

/*
Global variables used to store the lists of event statistics for all entries and sub areas.
*/
eventStats *listAll   = NULL;
eventStats *listSub1  = NULL;
eventStats *listSub2  = NULL;
eventStats totalAll,
           totalSub1,
           totalSub2;

/*
InitializeMetric initializes the Numbers structure for a given metric.
*/
void initializeMetric(Numbers *numbers)
{
	//declaring the Numbers structure
	numbers -> cap = 16;
	numbers -> count = 0;
	//allocating memory for the value array in the Numbers structure
	numbers -> value = malloc(numbers->cap *sizeof *numbers->value);

	//Validoting the memory allocation
	if(!numbers -> value)
	{
		//if the memory allocation fails, print an error message and exit the program
		perror("Malloc");
		exit(1);
	}

	//Initializing the other fields in the Numbers structure
	numbers -> sum = 0;
	numbers -> mean = 0;
	numbers -> min = INT_MAX;
	numbers -> max = 0;
	numbers -> standardDev = 0;
	numbers -> quart1 = numbers -> median = numbers -> quart3=0;
	// Setting the lower and upper bounds to 0
	pthread_mutex_init(&numbers -> mutex, NULL);
}

/*
The function statsBucket initializes an eventStats structure with a given label.
It sets the name of the eventStats structure to the label and initializes the
metric fields using the initializeMetric function.
*/
void statsBucket (eventStats *stats, const char *label)
{
	//allocating memory for the name field in the eventStats structure
	stats -> name = strdup(label);
	//Validoting the memory allocation
	initializeMetric(&stats -> dispatch);
	initializeMetric(&stats -> enroute);
	initializeMetric(&stats -> overall);
}

/*
The function appendMetric appends a metric value to the Numbers structure.
It locks the mutex to create thread safety, checks if the capacity of the
Numbers structure is enough, and if it isnt, reallocates memory to increase the capacity.
*/
void appendMetric(Numbers *caller, int num)
{
	//Locking the mutex to ensure thread safety
	pthread_mutex_lock(&caller -> mutex);

	//Check if the capacity is enough to hold the new value
	if (caller -> count == caller -> cap)
	{
		//If it not enough it doubles the capacity
		caller -> cap *= 2;

		//Reallocating memory for the value array in the Numbers structure
		int *tmp = realloc(caller -> value, caller -> cap * sizeof *caller -> value);
		//Validoting the memory allocation
		if (!tmp)
		{
			perror("realloc");
			exit(1);
		}
		//Assigning the reallocated memory to the value field in the Numbers structure
		caller -> value = tmp;
	}

	//Appending the new value to the value array in the Numbers structure
	caller -> value[caller -> count++] = num;
	//Updating the sum, min, and max fields in the Numbers structure
	caller -> sum += num;
	if (num < caller -> min)
	{
		caller -> min = num;
	}
	if (num > caller -> max)
	{
		caller -> max = num;
	}

	//Unlocking the mutex to let other threads access the Numbers structure
	pthread_mutex_unlock(&caller -> mutex);
}

//The function CompareInt is used to compare two integers for sorting purposes.
static int CompareInt(const void *first, const void *second)
{
	//Casting the void pointers to const int pointers and dereferencing them to get the integer values
	const int firstCompare = *(const int *)first;
	const int secondCompare = *(const int *)second;

	//Returning the difference between the two integers
	if(firstCompare == secondCompare)
	{
		return 0;
	}
	else if(firstCompare < secondCompare)
	{
		return -1;
	}
	else if(firstCompare > secondCompare)
	{
		return 1;
	}
}

/*
The function metricFinalize calculates the mean, standard deviation, quartiles,
and bounds for the given Numbers structure.
*/
void metricFinalize(Numbers *numbers)
{
	//Locking the mutex to ensure thread safety
	if(numbers -> count == 0)
	{
		return;
	}
	
	//Calculating the mean by dividing the sum by the count
	numbers -> mean = (double) numbers -> sum / numbers -> count;

	//Calculating the minimum and maximum values
	double variable = 0.0;

	//Calculating the standard deviation by iterating through the value array
	//and summing the squared differences from the mean
	for(int count = 0; count < numbers -> count; count++)
	{
		double down = numbers -> value[count] - numbers -> mean;
		variable += down * down;
	}
	//Calculating the standard deviation by taking the square root of the variance
	numbers -> standardDev = sqrt(variable / numbers -> count);

	//Setting the min and max values
	qsort(numbers -> value, numbers -> count, sizeof numbers -> value[0], CompareInt);

	//Setting the min and max values based on the sorted value array
	size_t q1 = numbers->count/4;
	size_t med = numbers -> count/ 2;
	size_t q3 = (3 * numbers -> count)/4;

	//Setting the min and max values based on the sorted value array
	numbers -> min = numbers -> value[0];
	numbers -> max = numbers -> value[numbers -> count - 1];
	numbers -> quart1 = numbers -> value[q1];
	numbers -> median = numbers -> value[med];
	numbers -> quart3 = numbers -> value[q3];
	numbers -> interQuart = numbers -> quart3 - numbers -> quart1;
	numbers -> lowerBound = numbers -> quart1 - 1.5 * numbers -> interQuart;
	numbers -> upperBound = numbers -> quart3 + 1.5 * numbers -> interQuart;

	//Setting the lower and upper bounds to the min and max values if they are out of range
	if(numbers -> lowerBound < numbers -> min)
	{
		numbers -> lowerBound = numbers -> min;
	}

	//Setting the upper bound to the max value if it is greater than the max value
	if(numbers -> upperBound > numbers -> max)
	{
		numbers -> upperBound = numbers -> max;
	}
}

/*
The function findOrCreate searches for an eventStats structure with a given label.
*/
eventStats *findOrCreate(eventStats **stats, int *inPointer, int *capacityOfPtr, const char *label)
{
	//Locking the mutex to ensure thread safety
	for(int count = 0; count < *inPointer; count++)
	{
		//Checking if the name of the eventStats structure matches the label
		if(strcmp((*stats)[count].name, label) == 0)
		{
			//If it matches, return the pointer to the eventStats structure
			return &(*stats)[count];
		}
	}

	//If it does not match, check if the capacity is enough to hold the new eventStats structure
	if(*inPointer == *capacityOfPtr)
	{
		//If it is not enough, double the capacity
		if(*capacityOfPtr == 0)
		{
			*capacityOfPtr = 16;
		}
		else
		{
			*capacityOfPtr = *capacityOfPtr * 2;
		}

		//Reallocating memory for the eventStats array
		eventStats *temp = realloc(*stats, *capacityOfPtr * sizeof(**stats));

		//Validoting the memory allocation
		if(!temp)
		{
			perror("Realloc");
			exit(1);
		}

		//Assigning the reallocated memory to the stats pointer
		*stats = temp;
	}

	//If the capacity is enough, create a new eventStats structure
	//and initialize it with the label
	eventStats *ptr = &(*stats)[*inPointer];
	//statsBucket initializes the eventStats structure with the label
	statsBucket(ptr, label);
	//Setting the name field in the eventStats structure to the label
	(*inPointer)++;
	return ptr;
}

/*
The function openHeader opens the header file and reads its contents.
It parses the header information and initializes the global variables
used to store the header information.
*/
void openHeader(const char *neededName, const char *headerPath)
{
	//Opens the header file in read only mode
	int openFile = open(headerPath, O_RDONLY);

	//Validates if the file was opened successfully
	if(openFile == -1)
	{
		printf("Error opening file");
		exit(1);
	}

	//Allocating memory for the headerName array
	char buffer[2048];
	//Reading the header file into the buffer
	ssize_t bytesBeingRead = read(openFile, buffer, sizeof(buffer));

	//Validating if the read operation was successful
	if(bytesBeingRead <= 0)
	{
		perror("Read Header");
		exit(1);
	}

	//Setting the last byte of the buffer to null character
	buffer[bytesBeingRead] = '\0';

	//Splitting the buffer into lines using strtok_r
	char *saveLine, *line = strtok_r(buffer, "\n", &saveLine);

	//Allocating memory for the headerName array
	while(line && numFields < 40)
	{
		char *saveColon;
		char *saveChar, *strLength = strtok_r(line, ":", &saveChar);
		char *name = saveChar;

		//Checks if *name is not null
		while(*name == ' ')
		{
			++name;
		}

		int length = strlen(name);

		//Checks if the last character of name is a carriage return
		while(length > 0 && name[length - 1] == ' ')
		{
			//If it is, set it to null character
			name[--length] = '\0';
		}

		int index = numFields;
		//Checks if the length of strLength is greater than 0
		width[index] = atoi(strLength);
		//Validating the width
		headerName[index] = strdup(name);

		numFields++;

		line = strtok_r(NULL,"\n", &saveLine);
	}

	//Closes the file
	close(openFile);

	//Setting offset to 0 for the first field 
	offset[0] = 0;

	//Loops through the fields to calculate the offsets and record length
	for(int count = 1; count < numFields; count++)
	{
		offset[count] = offset[count - 1] + width[count - 1];
	}
	recordLength = offset[numFields - 1] + width[numFields - 1];

	//Loops through the headerName array to find the indices of the fields
	for (int count = 0; count < numFields; count++)
	{
		//Checks if the field name is "received_datetime", "dispatch_datetime",
		if (strcmp(headerName[count], "received_datetime") == 0)
		{
			ReceivedID = count;
		}
		if (strcmp(headerName[count], "dispatch_datetime") == 0)
		{
			DispatchID = count;
		}
		if (strcmp(headerName[count], "enroute_datetime") == 0)
		{
			EnrouteID = count;
		}
		if (strcmp(headerName[count], "onscene_datetime") == 0)
		{
			onsceneID = count;
		}
	}

	//loops through the headerName array to find the index of the needed field
	for(int counter = 0; counter < numFields; ++counter)
	{
		//Checks if the last character of the header name is a carriage return
		size_t length = strlen(headerName[counter]);

		//If length and headerName equals to '\r', then set the last character to null
		if(length && headerName[counter][length - 1] == '\r')
		{
			headerName[counter][--length] = '\0';
		}
		
		//Checks if the headerName matches the neededName
		if(strcasecmp(headerName[counter], neededName) == 0)
		{
			indexHeader = counter;
		}

		if (strcasecmp(headerName[counter], "received_datetime")  == 0)
		{
			ReceivedID = counter;
		}
		if (strcasecmp(headerName[counter], "dispatch_datetime")  == 0)
		{
			DispatchID = counter;
		}
		if (strcasecmp(headerName[counter], "enroute_datetime")   == 0)
		{
			EnrouteID = counter;
		}
		if (strcasecmp(headerName[counter], "onscene_datetime")   == 0)
		{
			onsceneID = counter;
		}

		if( strcasecmp(headerName[counter], "call_type_final_desc") == 0)
		{
			FinalCallID = counter;
		}

		if( strcasecmp(headerName[counter], "call_type_original_desc") == 0)
		{
			OriginalCallID = counter;
		}
	}

	//Checks if the indexHeader is less than 0 or if both 
	//FinalCallID and OriginalCallID are less than 0
	if(indexHeader < 0 || (FinalCallID < 0 && OriginalCallID < 0))
	{
		fprintf(stderr, "header missing\n");
		exit(1);
	}

}

/*
The function initializeStats initializes the global variables used to store the
statistics for the total entries and sub areas.
*/
void initializeStats(const char *subset1, const char *subset2)
{
	//Duplicating the subset1 and subset2 strings to subArea1 and subArea2
	subArea1 = strdup(subset1);
	subArea2 = strdup(subset2);

	//statsBucket initializes the eventStats structure for totalAll, totalSub1, and totalSub2
	statsBucket(&totalAll, "TOTAL");
	statsBucket(&totalSub1, subArea1);
	statsBucket(&totalSub2, subArea2);
}

//Converts a time string in the format "MM/DD/YYYY HH:MM:SS AM/PM" to a time_t value.
//The function takes a pointer to the time string and its length as arguments.
static time_t convertingTime(const char *field, size_t length)
{
	//Stores the time in a struct tm structure
	struct tm time = {0};

	//Validating the length of the time string
	if(length < 21)
	{
		return -1;
	}
	
	//Checks if the first character of the time string is a digit
	time.tm_mon = 10 * (field[0] - '0') + (field[1] - '0') - 1;
	time.tm_mday = 10*(field[3]-'0') + (field[4]-'0');
	time.tm_year = 1000*(field[6]-'0') + 100*(field[7]-'0')+ 10*(field[8]-'0') + (field[9]-'0') - 1900;

	int hour  = 10*(field[11]-'0') + (field[12]-'0');
	const int pm = (field[20] == 'P');

	//Checks if the hour is 12 and sets it to 0 if pm 
	//is false, or adds 12 if pm is true
	if (hour == 12)
	{
		hour = pm ? 12 : 0;
	}
	else if (pm)
	{
		hour += 12;
	}

	//Sets the hour, minute, and second fields in the struct tm structure
	time.tm_hour = hour;
	time.tm_min  = 10 * (field[14]-'0') + (field[15]-'0');
	time.tm_sec  = 10 * (field[17]-'0') + (field[18]-'0');

	//returns the time in seconds since the epoch
	return timegm(&time);
}

//The function trimString trims leading and trailing spaces from a string.
static void trimString(char *string)
{
	//Checks if the string is not null
	while (*string == ' ')
	{
		++string;
	}
	//Finds the end of the string and trims trailing spaces
	char *end = string + strlen(string);

	//Checks if the end of the string is not null
	while (end > string && end[-1] == ' ')
	{
		--end;
	}

	//Sets the end of the string to null character
	*end = '\0';
}

//The function finalizeAll finalizes the metrics for all entries and sub areas.
void finalizeAll (void)
{
	/*
	Finalizes the metrics for totalAll, totalSub1, and totalSub2.
	Also finalizes the metrics for each entry in listAll, listSub1, and listSub2.
	*/
	metricFinalize(&totalAll.dispatch);
	metricFinalize(&totalAll.enroute);
	metricFinalize(&totalAll.overall);

	metricFinalize(&totalSub1.dispatch);
	metricFinalize(&totalSub1.enroute);
	metricFinalize(&totalSub1.overall);

	metricFinalize(&totalSub2.dispatch);
	metricFinalize(&totalSub2.enroute);
	metricFinalize(&totalSub2.overall);

	//Finalizes the metrics for each entry in listAll, listSub1, and listSub2
	for (int count = 0; count < AllEntries; count++)
	{
		metricFinalize(&listAll[count].dispatch);
		metricFinalize(&listAll[count].enroute);
		metricFinalize(&listAll[count].overall);
	}

	for (int count = 0; count < AllSubArea1; ++count)
	{
		metricFinalize(&listSub1[count].dispatch);
		metricFinalize(&listSub1[count].enroute);
		metricFinalize(&listSub1[count].overall);
	}

	for (int count = 0; count < AllSubArea2; ++count)
	{
		metricFinalize(&listSub2[count].dispatch);
		metricFinalize(&listSub2[count].enroute);
		metricFinalize(&listSub2[count].overall);
	}
}

//The function pick_metric returns a pointer to the Numbers structure for a given metric.
static Numbers *pick_metric(eventStats *event, Metric metric)
{
	//Checks the metric and returns the corresponding Numbers structure
	if(metric == metricDispatch)
	{
		return &event -> dispatch;
	}
	//Checks if the metric is enroute and returns the corresponding Numbers structure
	else if(metric == metricEnroute)
	{
		return &event -> enroute;
	}
	//If neither metricDispatch nor metricEnroute, returns the overall metric
	else
	{
		return &event -> overall;
	}
}

//The function printPage prints the statistics for a given metric and eventStats array.
//It takes a title, metric label, metric type, eventStats array, number of rows, and a total row as arguments.
static void printPage(const char *title,
                      const char *metricLabel,
                      Metric metric,
                      eventStats *rows, int nRows,
                      eventStats *totalRow)
{
	//Prints the title and metric label
	//%-40,23,etc are for spacing of the output on most, tjat or i could do it manually
	//Nah
	printf("\n%-40s  -  %s\n\n", title, metricLabel);
	puts("       Call Type        | count  | min |  lowerBound  | quart1 | med |  mean  |"
	     "  Q3 | UB  | max  | interQuart | Standard Deviation)");

	//Prints the header for the statistics table
	Numbers *print1 = pick_metric(totalRow, metric);
	printf("%23s|%7d |%5d|%5ld|%4lld|%5lld|%8.2f|%5lld|%5ld|%6d|%5ld|%8.2f\n",
	       "TOTAL",
	       print1 -> count, print1 -> min, print1 -> lowerBound, print1 -> quart1, print1 -> median,
	       print1 -> mean,  print1 -> quart3, print1 -> upperBound, print1 -> max, print1 -> interQuart, print1 -> standardDev);

	//Loops through the rows and prints the statistics for each event type
	for (int count = 0; count < nRows; ++count)
	{
		Numbers *print2 = pick_metric(&rows[count], metric);

		//Prints the statistics for each event type
		printf("%23s|%7d |%5d|%5ld|%4lld|%5lld|%8.2f|%5lld|%5ld|%6d|%5ld|%8.2f\n",
		       rows[count].name,
		       print2 -> count, print2 -> min, print2 -> lowerBound, print2 -> quart1, print2 -> median,
		       print2 -> mean,  print2 ->quart3, print2 -> upperBound, print2 -> max, print2 -> interQuart,
		       print2 -> standardDev);
	}
}


//The function printCallTypePage prints the statistics for a given call type.
//It takes a title, eventStats array, number of rows, and a total row as arguments.
//It prints the call type, count, min, max, mean, and standard deviation for each call type.
//It also prints the total statistics for all call types.
void printCallTypePage(const char *title,
                       eventStats *array,int n,
                       eventStats *total)
{
	//Prints the title for the call type statistics
	printf("\n%s\n", title);

	//Prints the header for the call type statistics table
	printf("%-18s %6s %6s %6s %8s %8s\n",
	       "Call-Type","n","min","max","mean","stdev");

	//Prints the total statistics for all call types
	printf("%-18s %6d %6d %6d %8.1f %8.1f\n",
	       "TOTAL",
	       total -> dispatch.count,
	       total -> dispatch.min,
	       total -> dispatch.max,
	       total -> dispatch.mean,
	       total -> dispatch.standardDev);

	//Loops through the array and prints the statistics for each call type
	for (int i=0; i<n; i++)
	{
		eventStats *caller = &array[i];
		printf("%-18s %6d %6d %6d %8.1f %8.1f\n",
		       caller -> name,
		       caller -> dispatch.count,
		       caller -> dispatch.min,
		       caller -> dispatch.max,
		       caller -> dispatch.mean,
		       caller -> dispatch.standardDev);
	}
}

//The function threading is the thread function that processes the records.
//It takes a pointer to a threadCount structure as an argument.
//It reads the records from the file, calculates the metrics, and appends them to the
//total statistics and the event statistics for each call type.
static void *threading(void *vp)
{
	//Calling a threadCount structure pointer
	threadCount *threadCounter = vp;
	//Allocating memory for the record
	char *rec = malloc(recordLength);

	if (!rec)
	{
		perror("malloc");
		exit(1);
	}

	//Validating the memory allocation
	for (long i = 0; i < threadCounter ->recordCount; ++i)
	{
		//Calculating the offset for the record
		off_t off = (threadCounter -> recordIndex + i) * recordLength;

		//Reading the record from the file using pread
		if (pread(threadCounter -> openFile, rec, recordLength, off) != recordLength)
		{
			perror("pread");
			exit(1);
		}

		//Each variable is used to store the time for each metric
		int timeForReceiving = convertingTime(rec + offset[ReceivedID], width[ReceivedID]);
		int timeForDispatch = convertingTime(rec + offset[DispatchID], width[DispatchID]);
		int timeForEnrout = convertingTime(rec + offset[EnrouteID], width[EnrouteID]);
		int timeForOnscene = convertingTime(rec + offset[onsceneID], width[onsceneID]);

		//Each variable is used to store the time difference between the metrics
		int dispatchReceived = timeForDispatch - timeForReceiving;
		int onsceneEnroute = timeForOnscene - timeForEnrout;
		int onsceneReceived = timeForOnscene - timeForReceiving;

		//Checks if the time for receiving, dispatch, enroute, and onscene is less than 0
		if(dispatchReceived < 0 || onsceneEnroute < 0 || onsceneReceived < 0)
		{
			continue;
		}

		//Appends the metrics to the total statistics
		appendMetric(&totalAll.dispatch, dispatchReceived);
		appendMetric(&totalAll.enroute,  onsceneEnroute);
		appendMetric(&totalAll.overall,  onsceneReceived);

		//Variable to store the description of the call type
		char description[128];
		//Copies the call type description from the record to the description variable
		memcpy(description, rec + offset[FinalCallID], width[FinalCallID]);
		//Sets the last character of the description to null character
		description[width[FinalCallID]] = '\0';
		//Trims the leading and trailing spaces from the description
		trimString(description);

		//Checks if the description is empty and copies the original call type description
		if( *description == '\0')
		{
			//Copies the original call type description from the record to the description variable
			memcpy(description, rec + offset[OriginalCallID], width[OriginalCallID]);
			//Sets the last character of the description to null character
			description[width[OriginalCallID]] = '\0';
			//Trims the leading and trailing spaces from the description
			trimString(description);
		}

		//Locks the mutex to ensure thread safety when accessing the listAll
		pthread_mutex_lock(&listMutex);
		//
		eventStats *eventPointer = findOrCreate(&listAll, &AllEntries, &capacityOfAll, description);

		//Unlocks the mutex after accessing the listAll
		pthread_mutex_unlock(&listMutex);

		//Appends the metrics to the event statistics for the call type
		appendMetric(&eventPointer -> dispatch, dispatchReceived);
		appendMetric(&eventPointer -> enroute, onsceneEnroute);
		appendMetric(&eventPointer -> overall, onsceneReceived);

		//Variable to store the area of the call
		char area[128];
		//Copies the area from the record to the area variable
		memcpy(area, rec + offset[indexHeader], width[indexHeader]);
		//Sets the last character of the area to null character
		area[width[indexHeader]] = '\0';
		//Stores the pointer to the area variable
		char *blankPointer = area;

		while (*blankPointer == ' ') 
		{
			++blankPointer;
		}

		//ln is used to store the length of the area string
		size_t areaLength = strlen(blankPointer);

		//Loops through the area string to remove trailing spaces
		while (areaLength && blankPointer[areaLength - 1] == ' ') 
		{
			blankPointer[--areaLength] = '\0';
		}

		//Checks if strcmp is used to compare the area with subArea1 and subArea2
		//If it is, appends the metrics to the total statistics for subArea1
		if (!strcmp(blankPointer, subArea1))
		{
			//Appends the metrics to the total statistics for subArea1
			appendMetric(&totalSub1.dispatch, dispatchReceived);
			appendMetric(&totalSub1.enroute,  onsceneEnroute);
			appendMetric(&totalSub1.overall,  onsceneReceived);

			//Locks the mutex to ensure thread safety when accessing the listSub1
			pthread_mutex_lock(&listMutex);
			//Finds or creates an eventStats structure for subArea1
			eventStats *structure = findOrCreate(&listSub1,
			                              &AllSubArea1,
			                              &capacityOfSub1,
			                              description);
			//Unlocks the mutex after accessing the listSub1
			pthread_mutex_unlock(&listMutex);

			//Appends the metrics to the event statistics for subArea1
			appendMetric(&structure->dispatch, dispatchReceived);
			appendMetric(&structure->enroute,  onsceneEnroute);
			appendMetric(&structure->overall,  onsceneReceived);
		}
		//Checks if strcmp is used to compare the area with subArea2
		//If it is, appends the metrics to the total statistics for subArea2
		else if (!strcmp(blankPointer, subArea2))
		{
			appendMetric(&totalSub2.dispatch, dispatchReceived);
			appendMetric(&totalSub2.enroute,  onsceneEnroute);
			appendMetric(&totalSub2.overall,  onsceneReceived);

			pthread_mutex_lock(&listMutex);
			eventStats *structure = findOrCreate(&listSub2,
			                              &AllSubArea2,
			                              &capacityOfSub2,
			                              description);
			pthread_mutex_unlock(&listMutex);

			appendMetric(&structure->dispatch, dispatchReceived);
			appendMetric(&structure->enroute,  onsceneEnroute);
			appendMetric(&structure->overall,  onsceneReceived);
		}

	}

	//Frees the allocated memory for the record
	free(rec);
	return NULL;
}

//The main function is the entry point of the program.
int main(int argc, char * argv[])
{

	//Creating variables to store the command line arguments
	const char *dataPath = argv[1];
	const char *headerPath = argv[2];
	int AllThreads = atoi(argv[3]);
	const char *subsetCol = argv[4];
	const char *subArea1CL = argv[5];
	const char *subArea2CL = argv[6];

	//Call function to open the header file and initialize the statistics
	openHeader(subsetCol, headerPath);
	initializeStats(subArea1CL, subArea2CL);

	//Declaring the variables to store the file descriptor and the number of records
	int openFile = open(dataPath, O_RDONLY);

	if(openFile < 0)
	{
		perror(dataPath);
		return 1;
	}

	//struct stat is used to get the size of the file
	struct stat st;
	//fstat is used to get the size of the file
	fstat(openFile, &st);
	//Calculating the number of records in the file
	const long nRecords = st.st_size / recordLength;
	const long chunk = nRecords / AllThreads;

	//**************************************************************
	// DO NOT CHANGE THIS BLOCK
	// Time stamp start
	struct timespec startTime;
	struct timespec endTime;

	clock_gettime(CLOCK_REALTIME, &startTime);
	//**************************************************************

	//Calls pthread_t to create threads for processing the records
	pthread_t tid[AllThreads];
	//Creating an array of threadCount structures to store the thread information
	threadCount threadCounter[AllThreads];

	for(int t = 0; t < AllThreads; ++t)
	{
		threadCounter[t].openFile = openFile;
		threadCounter[t].recordIndex  = t * chunk;
		threadCounter[t].recordCount  = (t == AllThreads-1) ?  nRecords - threadCounter[t].recordIndex
		                      :  chunk;
		pthread_create(&tid[t], NULL, threading, &threadCounter[t]);
	}

	//Joining the threads to ensure all threads complete before proceeding
	for (int t = 0; t < AllThreads; ++t)
	{
		pthread_join(tid[t], NULL);
	}

	//Finalizing the metrics for all entries and sub areas
	finalizeAll();

	//Printing the call type statistics for all entries and sub areas
	printPage("TOTAL", "Dispatch Time - Received Time",
	          metricDispatch, listAll, AllEntries, & totalAll);
	printPage("TOTAL", "OnScene  Time - Enroute Time",
	          metricEnroute, listAll, AllEntries, & totalAll);
	printPage("TOTAL", "OnScene  Time - Received Time",
	          metricOverall, listAll, AllEntries, & totalAll);

	printPage(subArea1, "Dispatch Time - Received Time",
	          metricDispatch, listSub1, AllSubArea1, &totalSub1);
	printPage(subArea1, "OnScene  Time - Enroute Time",
	          metricEnroute, listSub1, AllSubArea1, &totalSub1);
	printPage(subArea1, "OnScene  Time - Received Time",
	          metricOverall, listSub1, AllSubArea1, &totalSub1);

	printPage(subArea2, "Dispatch Time - Received Time",
	          metricDispatch, listSub2, AllSubArea2, &totalSub2);
	printPage(subArea2, "OnScene  Time - Enroute Time",
	          metricEnroute, listSub2, AllSubArea2, &totalSub2);
	printPage(subArea2, "OnScene  Time - Received Time",
	          metricOverall, listSub2, AllSubArea2, &totalSub2);

	//**************************************************************
	// DO NOT CHANGE THIS BLOCK
	// Clock output
	clock_gettime(CLOCK_REALTIME, &endTime);
	time_t sec = endTime.tv_sec - startTime.tv_sec;
	long n_sec = endTime.tv_nsec - startTime.tv_nsec;
	if (endTime.tv_nsec < startTime.tv_nsec)
	{
		--sec;
		n_sec = n_sec + 1000000000L;
	}

	printf("Total Time was %ld.%09ld seconds\n", sec, n_sec);
	//**************************************************************

	close(openFile);
	return 0;
}