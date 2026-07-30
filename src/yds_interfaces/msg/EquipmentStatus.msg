std_msgs/Header header

uint8 STATE_UNKNOWN=0
uint8 STATE_INITIALIZING=1
uint8 STATE_READY=2
uint8 STATE_RUNNING=3
uint8 STATE_WARNING=4
uint8 STATE_ERROR=5
uint8 STATE_CRITICAL=6
uint8 STATE_STOPPED=7

string equipment_id
uint8 state
int32 error_code
string message
