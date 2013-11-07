#ifndef ERROR_H
#define ERROR_H

namespace cbus
{
enum Error
{
    NO_ERROR = 0,
    TIMEOUT,
    INVALID_ARGUMENT,
    METHOD_CALL_FAILED,
    METHOD_DOES_NOT_EXIST,
    SERVICE_DOES_NOT_EXIST
};
}	//end namespace cbus

#endif // ERROR_H
