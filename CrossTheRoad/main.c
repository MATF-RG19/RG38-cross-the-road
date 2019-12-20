#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>
#include <time.h>

static float animation_ongoing = 0;
static float animation_parameter = 0;

const static float size = 0.2; // velicina pileta
const static float vector = 0.3; // velicina za koju se pile krece
static char side; // pravac i smer u kom se pomera pile (l || r || f || b)
static char current_step = 0; // trenutni potez koji je ucinjen - treba nam zbog rotacije pileta kad se krece u stranu

static float x_curr; // x koordinata pileta po ravni z0
static float y_curr; // y koordinata pileta po ravni z0

static int points; // trenutni broj poena (uvecava se za svako kretanje unapred)

static void on_reshape(int width, int height); 
static void on_display(void);
static void on_keyboard(unsigned char key, int x, int y);
static void on_timer(int value);

static void on_keyboard_for_arrows(int key, int x, int y); // osluskivac za strelice
static void initialize(void);
static void jumpCheck(unsigned char m); // funkcija koja proverava da li je moguce kretanje u zadatom pravcu
static void checkAndStartTimer(); // funkcija koja pokrece tajmer na prvi dodir nekog od tastera za kretanje
void drawChicken(); // funkcija za crtanje pileta
static void rotateChicken(char current_step); // funkcija koja rotira pile kad pri promeni pravca ili smera kretanja

#define TIMER_INTERVAL 20
#define TIMER_ID 0

int main(int argc, char** argv){
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    
    glutInitWindowSize(1000, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow(argv[0]);
    
    glutReshapeFunc(on_reshape);
    glutDisplayFunc(on_display);
    glutKeyboardFunc(on_keyboard);
    glutSpecialFunc(on_keyboard_for_arrows);
    
    glClearColor(0.75, 0.75, 0.75, 0);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    float light_position[] = {-1, 1, 1, 0};
    float light_ambient[] = {.3f, .3f, .3f, 1};
    float light_diffuse[] = {.7f, .7f, .7f, 1};
    float light_specular[] = {.7f, .7f, .7f, 1};

    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    
    initialize();
    
    glutMainLoop();
    
    return 0;
}

static void on_display(void){
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    gluLookAt(0, 3, 3,
              0, 0, 0,
              0, 1, 0);
    
    glColor3f(0, 0, 1);
    
    glTranslatef(x_curr, 0, -y_curr);
    drawChicken();
    
    glutSwapBuffers();
}

static void on_keyboard(unsigned char key, int x, int y){
    
    switch(key){
        
        case 27: // Esc
            exit(0);
            break;
        case 'q':
        case 'Q':
            animation_ongoing = 0;
        case 'r':
        case 'R':
            animation_parameter = 0;
            glutPostRedisplay();
    }
}

static void on_keyboard_for_arrows(int key, int x, int y){
    
    
    /////////////////////////////////////////////////////// postavljamo reakcije na strelice
    switch(key){
        case GLUT_KEY_LEFT: 
            side = 'l';
            jumpCheck(side);
            break;
        case GLUT_KEY_RIGHT: 
            side = 'r';
            jumpCheck(side);
            break;
        case GLUT_KEY_UP: 
            side = 'f';
            jumpCheck(side);
            break;
        case GLUT_KEY_DOWN: 
            side = 'b';
            jumpCheck(side);
            break;
    }
    ////////////////////////////////////////////////////////
}

static void jumpCheck(unsigned char m){ //////////////////////////////////////////////////////// kretanje pileta i granice kretanja
    
    switch(m){
        
        case 'l':
            if (x_curr - vector >= -(1.5 - size / 2)){
                x_curr -= vector;
            }
            current_step = 'l';
            checkAndStartTimer();
            glutPostRedisplay();
            break;
        case 'r':
            if (x_curr + vector <= 1.5 - size / 2){
                x_curr += vector;
            }
            current_step = 'r';
            checkAndStartTimer();
            glutPostRedisplay();
            break;
        case 'f':
            y_curr += vector;
            current_step = 'f';
            checkAndStartTimer();
            glutPostRedisplay();
            points ++;
            printf("Points: %d\n", points);
            break;
        case 'b':
            if (y_curr - vector >= -(1 - size / 2)){
                y_curr -= vector;
                points --;;
                printf("Points: %d\n", points);
            }
            current_step = 'b';
            checkAndStartTimer();
            glutPostRedisplay();
            break;
    }
    
    glutPostRedisplay();
}

static void initialize(void){
    
    x_curr = 0;
    y_curr = -1;
    
    points = 0;
    
}

static void on_reshape(int width, int height){
    
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(30, (float) width/height, 1, 1000);
}

static void on_timer(int value){
    
    if(value == TIMER_ID)
        animation_parameter += 1;
    
    if(animation_ongoing){
        glutTimerFunc(TIMER_INTERVAL, on_timer, TIMER_ID);
    }
}

static void checkAndStartTimer(){
    if(!animation_ongoing){
        animation_ongoing = 1;
        glutTimerFunc(TIMER_INTERVAL, on_timer, TIMER_ID);
    }
}

void drawChicken(){ // funkcija za crtanje pileta
    
    rotateChicken(current_step);
    
    glPushMatrix(); // glavni deo 1
        glColor3f(1, 1, 1);
        glScalef(0.5, 1, 0.7);
        glTranslatef(0, 0.03, 0);
        glutSolidCube(0.17);
    glPopMatrix();
    
    glPushMatrix(); // glavni deo 2
        glColor3f(1, 1, 1);
        glScalef(0.5, 0.4, 0.70);
        glTranslatef(0, -0.056, -0.04);
        glutSolidCube(0.17);
    glPopMatrix();
    
    glPushMatrix(); // leva noga
        glColor3f(1, 0.4, 0);
        glScalef(0.5, 1, 0.5);
        glTranslatef(0.05, -0.07, -0.01);
        glutSolidCube(0.03);
    glPopMatrix();
    
    glPushMatrix(); // desna noga
        glColor3f(1, 0.4, 0);
        glScalef(0.5, 1, 0.5);
        glTranslatef(-0.05, -0.07, -0.01);
        glutSolidCube(0.03);
    glPopMatrix();
    
    glPushMatrix(); // levo oko
        glColor3f(0, 0, 0);
        glTranslatef(0.043, 0.07, 0.02);
        glutSolidCube(0.015);
    glPopMatrix();
    
    glPushMatrix(); // desno oko
        glColor3f(0, 0, 0);
        glTranslatef(-0.043, 0.07, 0.02);
        glutSolidCube(0.015);
    glPopMatrix();
    
    glPushMatrix(); // gornji kljun
        glColor3f(1, 0.4, 0);
        glTranslatef(0, 0.065, 0.075);
        glutSolidCube(0.03);
    glPopMatrix();
    
    glPushMatrix(); // donji kljun
        glColor3f(1, 0, 0);
        glTranslatef(0, 0.04, 0.068);
        glutSolidCube(0.02);
    glPopMatrix();
    
    glPushMatrix(); // levo krilo
        glColor3f(1, 1, 1);
        glScalef(0.25, 0.2, 0.35);
        glTranslatef(0.17, -0.12, -0.08);
        glutSolidCube(0.17);
    glPopMatrix();
    
    glPushMatrix(); // desno krilo
        glColor3f(1, 1, 1);
        glScalef(0.25, 0.2, 0.35);
        glTranslatef(-0.17, -0.12, -0.08);
        glutSolidCube(0.17);
    glPopMatrix();
    
    glPushMatrix(); // kresta
        glColor3f(1, 0, 0);
        glScalef(0.3, 0.3, 1);
        glTranslatef(0, 0.40, 0);
        glutSolidCube(0.05);
    glPopMatrix();
}

static void rotateChicken(char current_step){
    
    glRotatef(180, 0, 1, 0);
    
    if(current_step == 'l')
        glRotatef(90, 0, 1, 0);
    else if(current_step == 'r')
        glRotatef(-90, 0, 1, 0);
    else if(current_step == 'b')
        glRotatef(180, 0, 1, 0);
}


