#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/ioctl.h>
#include <stdio.h>
#include <cstdlib>
#include <termios.h>
#include <fcntl.h>

void setNonBlockingInput(bool enable) {
    static termios oldt, newt;
    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO); // nyers mód
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK); // ne várjon bemenetre
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // visszaállít
    }
}

struct Window {
  struct{
    int cols;
    int lines;
  } size;
  int matrix[1000][1000];
};

struct Object {
  int pos_line;
  int pos_col;
  int id;
} apple;

struct ObjectList {
  int num_objects;
  Object objects[1000];
  int direction;
} snake, items;
// 1 up, 2 left, 3 down, 4 right

int refresh_rate = 3000000;

Window window_start() {
  Window window;
  int cols, lines;

  #ifdef TIOCGSIZE
    struct ttysize ts;
    ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
    cols = ts.ts_cols;
    lines = ts.ts_lines;
  #elif defined(TIOCGWINSZ)
    struct winsize ts;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
    cols = ts.ws_col;
    lines = ts.ws_row;
  #endif /* TIOCGSIZE */
  
  window.size.cols = cols;
  window.size.lines = lines;

  return window;
}

Window window = window_start();

void object_random(Object &object, int id) {
  object.pos_line = std::rand()%window.size.lines - 1;
  object.pos_col = std::rand()%window.size.cols - 1;
  if(object.pos_line >= window.size.lines
      || object.pos_col >= window.size.cols)
    object_random(object, id);
  object.id = id;
 }

void output_window() {
  system("clear");
  for(int i = 0; i < window.size.lines; i++) {
    for(int j = 0; j < window.size.cols; j ++)
      if(window.matrix[i][j] == 0)
        std::cout << ' ';
        else if(window.matrix[i][j] == 2)
          std::cout << '[';
        else std::cout << '#';
    std::cout << '\n';
  }
}

void draw_object(Object object) {



  window.matrix[object.pos_line][object.pos_col] = object.id;
}

void initialize() {
  setNonBlockingInput(true);
  object_random(apple, 1);
  items.num_objects ++;

  object_random(snake.objects[0], 2);
  snake.num_objects ++;

  draw_object(apple);
  for(int i = 0; i < snake.num_objects; i++)
    draw_object(snake.objects[i]);
  snake.direction = 1;
}

void clear_object(Object object) {
  window.matrix[object.pos_line][object.pos_col] = 0;
}

void new_frame() {
  clear_object(apple);
  object_random(apple, 1);
  for(int i = 0; i < snake.num_objects; i++)
    clear_object(snake.objects[i]);

  switch (snake.direction) {
    case 1: {
              snake.objects[0].pos_line--;
            }
          break;
  }
  draw_object(apple);
  for(int i = 0; i < snake.num_objects; i++)
    draw_object(snake.objects[i]);

}

void gameover() {
  setNonBlockingInput(false);
  std::cout << "Game over!";
  exit(1);
}

int main() {
initialize();

output_window();
usleep(refresh_rate/5);

char input;

while(true) {
  if(read(STDIN_FILENO, &input, 1) > 0)
    gameover();

  new_frame();
  output_window();
  usleep(refresh_rate/5);
  }
}
