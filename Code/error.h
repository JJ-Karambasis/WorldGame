#ifndef ERROR_H
#define ERROR_H

struct error_stream : public string_stream
{
    b32 HasErrorOccured;
};

global error_stream* __Internal_Global_ErrorStream__;

inline void SetGlobalErrorStream(error_stream* Stream)
{
    __Internal_Global_ErrorStream__ = Stream;
}

inline error_stream* GetGlobalErrorStream()
{
    return __Internal_Global_ErrorStream__;
}

inline error_stream CreateErrorStream()
{
    error_stream Stream = {};
    CreateStringStream(&Stream, KILOBYTE(16));
    return Stream;
}

#define WRITE_ERROR(format, ...) \
do \
{ \
    Write(GetGlobalErrorStream(), "ERROR(file: %s, function: %s, line: %d): ", CURRENT_FILENAME, CURRENT_FUNCTION, CURRENT_LINE); \
    Write(GetGlobalErrorStream(), format, __VA_ARGS__); \
    EndLine(GetGlobalErrorStream()); \
    GetGlobalErrorStream()->HasErrorOccured = true; \
} while(0)

#define WRITE_AND_HANDLE_ERROR(format, ...) \
do \
{ \
    Write(GetGlobalErrorStream(), "ERROR(file: %s, function: %s, line: %d): ", CURRENT_FILENAME, CURRENT_FUNCTION, CURRENT_LINE); \
    Write(GetGlobalErrorStream(), format, __VA_ARGS__); \
    EndLine(GetGlobalErrorStream()); \
    GetGlobalErrorStream()->HasErrorOccured = true; \
    goto handle_error; \
} while(0)

#define BOOL_CHECK_AND_HANDLE(null_check, format, ...) \
do \
{ \
    if(!(null_check)) \
        WRITE_AND_HANDLE_ERROR(format, __VA_ARGS__); \
} while(0)

#endif