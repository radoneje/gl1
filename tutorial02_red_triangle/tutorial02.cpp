#include <cstdlib>
#include <iostream>
using namespace std;

/* Use glew.h instead of gl.h to get all the GL prototypes declared */
#include <GL/glew.h>
#include <GL/freeglut.h>
/* Using SDL2 for the base window and OpenGL context init */
void display()
{
    /* рисуем что нибудь */
}
int main(int argc, char **argv)
{
    glutInit(&argc, argv);//начальная инициализация окна
    glutInitDisplayMode(GLUT_DOUBLE);//установка режима отображения
    glutInitWindowSize(400,400);//размер окна
    glutInitWindowPosition(200, 200);//начальная позиция на экране
    //вы заметили что вначале идут функции с приставкой glutInit...?, так вот они должны быть первыми, а потом уже все остальные ф-ии.
    glutCreateWindow("Window");//заголовок окна
    glClearColor(1, 1, 1, 0);//цвет фона

    // настройка проекции, с этими двумя ф-ми познакомимся поближе чуть позже.
    glMatrixMode(GL_PROJECTION);//режим матрицы
    glLoadIdentity();//отчищает матрицу

    glOrtho(-100, 100, -100, 100, -100, 100);//cоздаем пространство нашей сцены, в данном случае 3D пространство с высотой, шириной и глубиной в 200 едениц.
    glutDisplayFunc(display);//функция которой мы передаем имя функции для отрисовки окна.
    glutMainLoop();//запускаем всё проинициализированное, проще говоря та же ф-я main, только в данном случае glut'овская ф-я main.
    return 0;
}