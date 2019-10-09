#define __USE_GNU
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#define O_TMPFILE   (020000000 | __O_DIRECTORY)

int main(int argc, char const *argv[]) {
  int fd = open("/dev/shm", O_TMPFILE|O_EXCL|O_RDWR|O_APPEND ,600);

  fprintf(stderr, "%ld\n", lseek(fd, 0, SEEK_CUR));
  pid_t pid = fork();
  if (pid == 0){
    // child
    if (write(fd, "xyz\n", 4) != 4) {
      fprintf(stderr, "didn't write 4\n");
    }
  } else {
    // parent
    if (waitpid(pid, NULL, 0) != pid) {
      perror("waitpid: ");
      exit(1);
    }
    fprintf(stderr, "%ld\n", lseek(fd, 0, SEEK_CUR));
  }
  close(fd);
  return 0;
}
