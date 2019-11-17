#include <stdio.h>
#include <GL/glut.h>

void on_display(void){
    glClear(GL_COLOR_BUFFER_BIT);
    
    
    
    glutSwapBuffers();
}

int main(int argc, char** argv){
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    
    glutInitWindowSize(1000, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow(argv[0]);
    
    glutDisplayFunc(on_display);
    
    glClearColor(0.75, 0.75, 0.75, 0);
    
    glutMainLoop();
    
    return 0;
}
