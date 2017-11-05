//example 1: Create a thread

#include <stdio.h>

#include <pthread.h>
void* printx(void* unused)
{
          while(1)
          {
                   fputc('x',stdout); //Hàm xử lý của luồng in ký tự 'x'
          }
          return NULL;      
}
int mainaaaaaaaaaaaaaaa(int argc, char** argv)
{
pthread_t thread_id;
pthread_create(&thread_id, NULL, &printx, NULL); //Tạo luồng
while(1)
{
           fputc('o',stdout); //Luồng chính liên tục in ra ký tự 'o'
}
return 0;
}