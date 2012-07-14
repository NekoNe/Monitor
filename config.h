
enum { OK, FAIL, NOMEM, SYNTAX_FAIL }
    error_codes;


#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L

#include <stdbool.h>

#else

#undef bool
#undef true
#undef false

#define bool unsigned char
#define true  1
#define false 0

#endif


#define MONITOR_DEBUG_LEVEL_1 1
#define MONITOR_DEBUG_LEVEL_2 1
#define MONITOR_DEBUG_LEVEL_3 1

#define TOPIC_DEBUG_LEVEL_1 1
#define TOPIC_DEBUG_LEVEL_2 1
#define TOPIC_DEBUG_LEVEL_3 0

#define AGENT_DEBUG_LEVEL_1 1
#define AGENT_DEBUG_LEVEL_2 1

#define AGENT_USER_CONTROL_1 0

#define TOPIC_ID_SIZE 12 /* Exmaple "000.000.000\0" */
#define ID_DIGIT_SIZE 3
#define ID_DIGIT_NUMBER 3
#define ID_SEPAR_SIZE 1

#define ARRAY_WINDOW_SIZE 10
#define MAX_NAME_LENGTH 500
#define FUNCTION_NAME_SIZE 20
#define MAX_URL_SIZE 200
#define GENERIC_TOPIC_ID "000.000.000"

/**** names of requests ****/

/* returns list of topic's child. like 'ls' but 'lt' =) */
#define LIST_TOPIC "list_topic"


