#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstdlib>
#include <termios.h>
#include <fcntl.h>
#include <fstream>

int score = 0;
int highScore;

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
  int matrix[100][100];
};

struct Object {
  int pos_line;
  int pos_col;
  int id;
  bool updated;
} apple;

struct ObjectList {
  int num_objects;
  Object objects[1000];
  int direction;
} snake, items;
// 1 up, 2 left, 3 down, 4 right

int refresh_rate = 100000;

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
  object.pos_line = std::rand() % (window.size.lines - 1);
  object.pos_col = std::rand() % (window.size.cols - 1);
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
          std::cout << '0';
      else std::cout << '#';
    std::cout << '\n';
  }
}

void output_debug() {
  system("clear");
  for(int i = 0; i < window.size.lines; i++) {
    for(int j = 0; j < window.size.cols; j ++)
        std::cout << window.matrix[i][j];
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
  apple.updated = 0;

  object_random(snake.objects[0], 2);
  snake.num_objects = 1;

  draw_object(apple);
  for(int i = 0; i < snake.num_objects; i++)
    draw_object(snake.objects[i]);
  snake.direction = 0;

  std::ifstream file("snake.score");
  file >>highScore;
  file.close();
}

void clear_object(Object object) {
  window.matrix[object.pos_line][object.pos_col] = 0;
}

void gameover() {
  if(score >highScore) {
  std::ofstream file;
  file.open("snake.score", std::ofstream::out | std::ofstream::trunc);
  file << score;
  highScore = score;
  }


  setNonBlockingInput(false);
  system("clear");
  std::cout << "Game over!";
  std::cout << '\n';
  std::cout << "Score: " << score << '\n';
  std::cout << "High score: " << highScore << '\n';
  exit(0);
}

void check_wrap(Object object) {
  if(object.pos_col >= window.size.cols
    || object.pos_line >= window.size.lines
    || object.pos_line <= 0
    || object.pos_col < 0)
    gameover();
}

bool check_collisionObject(Object object1, Object object2) {
  if(object1.pos_col == object2.pos_col
    && object1.pos_line == object2.pos_line
  )
    return 1;
  else return 0;
}

void new_frame() {
  for(int i = 0; i<100; i++)
    for(int j = 0; j < 100; j++)
      window.matrix[i][j] = 0;

  switch (snake.direction) {
    case 1: {
      snake.objects[0].pos_line--;
    }
    break;
    case 2: {
      snake.objects[0].pos_col--;
    }
    break;
    case 3: {
      snake.objects[0].pos_line++;
    }
    break;
    case 4: {
      snake.objects[0].pos_col++;
    }
    break;
}
  check_wrap(snake.objects[0]);

  for(int i = 0; i < snake.num_objects; i++)
    draw_object(snake.objects[i]);

  draw_object(apple);
  if(check_collisionObject(snake.objects[0], apple)) {
    score ++;
    apple.updated = 1;
  }
}

void handle_input(char input) {
  switch (input) {
    case 'w': {
              //snake.objects[0].pos_line--;
              snake.direction = 1;
            }
          break;
    case 'a': {
                //snake.objects[0].pos_col--;
                snake.direction = 2;
              }
          break;
    case 's': {
                //snake.objects[0].pos_line++;
                snake.direction = 3;
              }
          break;
    case 'd': {
                //snake.objects[0].pos_col++;
                snake.direction = 4;
              }
          break;
  }

  check_wrap(snake.objects[0]); //maybe fix
}

int main() {
initialize();

output_window();
usleep(refresh_rate);

char input, latestInput;

while(true) {
  while (read(STDIN_FILENO, &input, 1) > 0) {
    latestInput = input;
  }

  if (latestInput != 0) {
    handle_input(latestInput);
    latestInput = 0;
  }

  if (apple.updated) {
    clear_object(apple);
    object_random(apple, 1);
    draw_object(apple);
    apple.updated = 0;
  }

  new_frame();
  output_window();
  usleep(refresh_rate);
  }
}
