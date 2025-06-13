#include <time.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
  srand(time(NULL));
  
  int answer = -1;
  int to_guess = rand()%100;
  
  while (answer != to_guess) {
    printf("Guess the number: ");
    scanf("%d", &answer);
    
    if (answer < to_guess) {
      printf("too low!\n");
    } else
    if (answer > to_guess) {
      printf("too high!\n");
    }
  }
  printf("congratulations! You found the number\n");
  return 0;
}