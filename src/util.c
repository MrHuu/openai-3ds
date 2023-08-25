#include "util.h"
#include "sys.h"

void UTIL_updateString(char** str, const char* newContent) {
    size_t newLength = strlen(newContent);
    
    char* newStr = realloc(*str, (newLength + 1) * sizeof(char));  // Reallocate memory for the new string
    
    if (newStr != NULL) {
        *str = newStr;  // Update the pointer with the reallocated memory
        
        strcpy(*str, newContent);  // Replace the content with the new string
        
//        printf("After reallocation: %s\n", *str);
    } else {
//        printf("Failed to reallocate memory.\n");
//        free(*str);  // Free the original memory if reallocation fails
//        *str = NULL;  // Set the pointer to NULL to indicate failure
		sys_error(1,"malloc failed!\n");
    }
}

void UTIL_appendString(char** str, const char* appendContent) {
    size_t originalLength = strlen(*str);
    size_t appendLength = strlen(appendContent);
    size_t newLength = originalLength + appendLength;
    
    char* newStr = realloc(*str, (newLength + 1) * sizeof(char));  // Reallocate memory for the combined string
    
    if (newStr != NULL) {
        *str = newStr;  // Update the pointer with the reallocated memory
        
        strcat(*str, appendContent);  // Append the new content to the original string
        
//        printf("After appending: %s\n", *str);
    } else {
        printf("Failed to reallocate memory.\n");
		sys_error(1,"malloc failed!\n");
        // You may choose to keep the original string as it is in case of reallocation failure
    }
}

char * new_str;
char* UTIL_string_concat(char *str1,char *str2)
{
//   char * new_str;
   
   if (new_str != NULL)
      free(new_str);

   if((new_str = malloc(strlen(str1)+strlen(str2)+1)) != NULL){
      new_str[0] = '\0';
      strcat(new_str,str1);
      strcat(new_str,str2);
   }
   else
      sys_error(1,"malloc failed!\n");

   return new_str;
}