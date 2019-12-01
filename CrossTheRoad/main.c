#include <stdio.h>
#include <GL/glut.h>

const static float size = 0.2; // velicina pileta

static float x_curr; // x koordinata pileta 
static float y_curr; // y koordinata pileta

static float vector = 0.25; // vektor kretanja
char side; // pravac i smer u kom se pomera pile

static void on_display(void);
static void on_keyboard(unsigned char key, int x, int y);
static void on_keyboard_for_arrows(int key, int x, int y);
static void initialize(void);

static void jump(unsigned char m);

int main(int argc, char** argv){
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    
    glutInitWindowSize(1000, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow(argv[0]);
    
    glutDisplayFunc(on_display);
    glutKeyboardFunc(on_keyboard);
    glutSpecialFunc(on_keyboard_for_arrows);
    
    glClearColor(0.75, 0.75, 0.75, 0);
    
    initialize();
    
    glutMainLoop();
    
    return 0;
}

static void on_display(void){
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glColor3f(0, 0, 1);
    glBegin(GL_POLYGON);
        glVertex3f(x_curr - size / 2, y_curr - size / 2, 0);
        glVertex3f(x_curr + size / 2, y_curr - size / 2, 0);
        glVertex3f(x_curr + size / 2, y_curr + size / 2, 0);
        glVertex3f(x_curr - size / 2, y_curr + size / 2, 0);
    glEnd();
    
    glutSwapBuffers();
}

static void on_keyboard(unsigned char key, int x, int y){
    
    switch(key){
        
        case 27: // Esc
            exit(0);
            break;
    }
}

static void on_keyboard_for_arrows(int key, int x, int y){
    
    switch(key){
        case GLUT_KEY_LEFT: 
            side = 'l';
            jump(side);
            break;
        case GLUT_KEY_RIGHT: 
            side = 'r';
            jump(side);
            break;
        case GLUT_KEY_UP: 
            side = 'f';
            jump(side);
            break;
        case GLUT_KEY_DOWN: 
            side = 'b';
            jump(side);
            break;
    }
}

static void jump(unsigned char m){
    
    switch(m){
        
        case 'l':
            if (x_curr - vector >= -(1 - size / 2))
                x_curr -= vector;
            break;
        case 'r':
            if (x_curr + vector <= 1 - size / 2)
                x_curr += vector;
            break;
        case 'f':
            if (y_curr + vector <= 1 - size / 2)
                y_curr += vector;
            break;
        case 'b':
            if (y_curr - vector >= -(1 - size / 2))
                y_curr -= vector;
            break;
    }
    
    glutPostRedisplay();
}

static void initialize(void){
    
    x_curr = 0;
    y_curr = -(1 - size / 2);
}
