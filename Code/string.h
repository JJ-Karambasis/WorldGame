#ifndef STRING_H
#define STRING_H

ptr LiteralStringLength(char* Data)
{
    ptr Result = 0;
    while(*Data++)
        Result++;
    return Result;
}

struct string
{
    char* Data;
    ptr Length;
    
    inline char operator[](ptr Index)
    {
        return Data[Index];
    }
};

struct string_stream
{
    arena Storage;
    ptr Capacity;
    ptr Size;
    char* Data;       
};

inline b32 StringEquals(char* A, ptr ALength, char* B, ptr BLength)
{
    if(ALength != BLength)
        return false;
    
    for(u32 Index = 0; Index < ALength; Index++)
    {
        if(A[Index] != B[Index])
            return false;
    }    
    return true;
}

inline b32 StringEquals(char* A, char* B)
{
    b32 Result = StringEquals(A, LiteralStringLength(A), B, LiteralStringLength(B));
    return Result;
}

string LiteralString(char* Data, ptr StringLength);
inline string operator+(string Left, int Right)
{
    string Result = LiteralString(Left.Data+Right, Left.Length-Right);
    return Result;
}

inline ptr operator-(string Left, char* Right)
{
    ptr Result = Left.Data - Right;
    return Result;
}

inline ptr operator-(string Left, string Right)
{
    ptr Result = Left.Data - Right.Data;
    return Result;
}

inline string LiteralString(char* Data, ptr StringLength)
{
    string Result;
    Result.Data = Data;
    Result.Length = StringLength;
    return Result;
}

inline string LiteralString(char* Data)
{
    string Result = LiteralString(Data, LiteralStringLength(Data));
    return Result;
}

inline string AllocateLiteralString(ptr StringLength, arena* Arena = GetDefaultArena())
{
    string Result;
    Result.Data = (char*)PushSize(Arena, StringLength+1, Clear, 0);
    Result.Data[StringLength] = 0;
    Result.Length = StringLength;
    return Result;
}

inline string PushLiteralString(char* String, ptr StringLength, arena* Arena = GetDefaultArena())
{
    string Result = AllocateLiteralString(StringLength, Arena);
    CopyMemory(Result.Data, String, StringLength);
    return Result;
}

inline string PushLiteralString(char* String, arena* Arena = GetDefaultArena())
{
    string Result = PushLiteralString(String, LiteralStringLength(String), Arena);
    return Result;
}

inline char* GetEnd(string String)
{
    char* Result = String.Data + String.Length-1;
    return Result;
}

string FindLastChar(string String, char Char)
{
    char* At = GetEnd(String);
    char* End = At;
    while(*End)
    {
        if(*End == Char)
        {
            string Result = LiteralString(End, (At-End)+1); 
            return Result;
        }
        End--;
    }      
    
    return {};    
}

char* FindLastChar(char* String, char Char)
{
    string Result = FindLastChar(LiteralString(String), Char);
    return Result.Data;
}

inline string GetFilename(string Path)
{
    string Result = FindLastChar(Path, OS_PATH_DELIMITER);
    if(Result.Data == NULL)
        return Path;
    return Result+1;
}

inline string GetFilePath(string Path)
{
    string Filename = GetFilename(Path);
    string Result = LiteralString(Path.Data, Filename-Path);
    return Result;
}

inline string Concat(char* A, ptr ALength, char* B, ptr BLength, arena* Arena = GetDefaultArena())
{
    ptr StringLength = ALength + BLength;
    string Result;
    Result.Data = (char*)PushSize(Arena, StringLength+1, Clear, 0);
    
    CopyMemory(Result.Data, A, ALength);
    CopyMemory(Result.Data+ALength, B, BLength);    
    Result.Data[StringLength] = 0;                   
    Result.Length = StringLength;
    return Result;
}

inline string Concat(const char* A, ptr ALength, const char* B, ptr BLength, arena* Arena = GetDefaultArena())
{
    string Result = Concat((char*)A, ALength, (char*)B, BLength, Arena);
    return Result;
}

inline string Concat(string A, char* B, ptr BLength, arena* Arena = GetDefaultArena())
{
    string Result = Concat(A.Data, A.Length, B, BLength, Arena);
    return Result;        
}

inline string Concat(string A, char* B, arena* Arena = GetDefaultArena())
{
    string Result = Concat(A.Data, A.Length, B, LiteralStringLength(B), Arena);        
    return Result;
}

inline string Concat(string A, string B, arena* Arena = GetDefaultArena())
{
    string Result = Concat(A.Data, A.Length, B.Data, B.Length, Arena);
    return Result;
}

inline string Concat(char* A, string B, arena* Arena = GetDefaultArena())
{
    string Result = Concat(A, LiteralStringLength(A), B.Data, B.Length, Arena);
    return Result;
}

inline string Concat(string A, char B, arena* Arena = GetDefaultArena())
{
    string Result = Concat(A.Data, A.Length, &B, 1, Arena);
    return Result;
}

inline string Concat(char* A, char B, arena* Arena = GetDefaultArena())
{
    string Result = Concat(A, LiteralStringLength(A), &B, 1, Arena);
    return Result;
}

inline void CreateStringStream(string_stream* Stream, ptr BlockSize)
{    
    Stream->Storage = CreateArena(BlockSize);    
}

inline string_stream CreateStringStream(ptr BlockSize)
{
    string_stream Result = {};
    CreateStringStream(&Result, BlockSize);
    return Result;
}

inline void CheckStringStreamSize(string_stream* Stream)
{
    b32 ShouldResize = !Stream->Capacity || (Stream->Size >= Stream->Capacity-1024);
    
    if(ShouldResize)
    {
        ptr NewCapacity = Stream->Capacity*2;
        if(!NewCapacity) 
            NewCapacity = 2048;
        
        char* Temp = PushArray(&Stream->Storage, NewCapacity, char, Clear, 0);
        CopyMemory(Temp, Stream->Data, Stream->Capacity);
        Stream->Data = Temp;
        Stream->Capacity = NewCapacity;
    }
}

inline void Write(string_stream* Stream, char* Format, va_list Args)
{
    CheckStringStreamSize(Stream);
    Stream->Size += vsprintf(Stream->Data+Stream->Size, Format, Args);
}

inline void Write(string_stream* Stream, char* Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    Write(Stream, Format, Args);
    va_end(Args);
}

inline void WriteChar(string_stream* Stream, char Value)
{
    CheckStringStreamSize(Stream);
    Stream->Data[Stream->Size++] = Value;
}

inline void EndLine(string_stream* Stream)
{
    WriteChar(Stream, '\n');
}

inline void WriteLine(string_stream* Stream, char* Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    Write(Stream, Format, Args);
    va_end(Args);
    EndLine(Stream);
}

inline string GetString(string_stream* Stream)
{
    string Result = LiteralString(Stream->Data, Stream->Size);
    return Result;
}

inline string PushString(string_stream* Stream, arena* Arena = GetDefaultArena())
{
    string Result = PushLiteralString(Stream->Data, Stream->Size, Arena);
    return Result;
}

#endif