#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>
#include <time.h>
#include <stdbool.h>

static float animation_ongoing = 0;
static float animation_parameter = 0;

// koordinata prednje ivice pileta na pocetku z = 0.91125
// koordinata zadnje ivice pileta na pocetku z = 1.08875
// koordinata desne ivice pileta na pocetku x = 0.06375
// koordinata leve ivice pileta na pocetku x = -0.06375

const static float sizeZ = 0.1775; // duzina pileta po z osi je 0.1775
const static float sizeX = 0.1275; // sirina pileta po x osi je 0.1275
const static float vector = 0.2; // velicina za koju se pile krece
static char side; // pravac i smer u kom se pomera pile (l || r || f || b)
static char current_step = 0; // trenutni potez koji je ucinjen - treba nam zbog rotacije pileta kad se krece u stranu
static int current_last_row = 19; // red polja nakon kog treba dodati novi red polja

static int x_curr; // x koordinata pileta
static int z_curr; // z koordinata pileta

static int points; // trenutni broj poena (uvecava se za svako kretanje unapred)
static int counter; // brojac koji treba da odluci kada pocinjemo sa dodavanjem novih polja
static int jump_back_counter; // brojac skokova nazad (dozvoljeno 3 skoka unazad)
static bool end; // istinitosna vrednost koja nam govori kada je kraj igre

static void on_reshape(int width, int height); 
static void on_display(void);
static void on_keyboard(unsigned char key, int x, int y);
static void on_timer(int value);

static void on_keyboard_for_arrows(int key, int x, int y); // osluskivac za strelice
static void initialize(void);
static void jumpCheck(unsigned char m); // funkcija koja proverava da li je moguce kretanje u zadatom pravcu
static void checkAndStartTimer(); // funkcija koja pokrece tajmer na prvi dodir nekog od tastera za kretanje
static void drawChicken(); // funkcija za crtanje pileta
static void rotateChicken(char current_step); // funkcija koja rotira pile kad pri promeni pravca ili smera kretanja
static void drawTree(); // funkcija koja crta drvo
static void drawCar(); // funkcija koja crta automobil
static void initialize_fields(); // funkcija koja inicijalizuje strukturu za polja
static void setCar(int coordinate1, int coordinate2); // postavlja automobil na odredjeno polje i postavlja vrednost polja na zauzeto
static void setTree(int coordinate1, int coordinate2); // postavlja drvo na odredjeno polje i postavlja vrednost polja na zauzeto
static bool isFree(int coordinate1, int coordinate2); // proverava da li je slobodno polje na koje pile zeli da skoci
static void addRow(int i); // funkcija koja uklanja red polja sa pocetka i dodaje novi na kraj
static int nextX(int currentI); // funkcija koja nam govori koji je sledeci indeks reda na koji mozemo da skocimo

#define TIMER_ID 0
#define TIMER_INTERVAL 20

typedef struct field_struct{
    bool taken;
    float x_coordinate[2];
    float middle_of_X;
    float z_coordinate[2];
    float middle_of_Z;
    char surface; // tip podloge (moze biti trava 'g', asfalt 'a' i voda 'w')
}field_struct;

field_struct field[20][15];

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
    
    glClearColor(0, 0.3, 0, 0);
    
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
    
    glPushMatrix();
        glTranslatef(field[x_curr][z_curr].middle_of_X, 0, field[x_curr][z_curr].middle_of_Z);
        drawChicken();
    glPopMatrix();
    
    setCar(5, 6);
    setTree(5, 8);
        
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
}

static void jumpCheck(unsigned char m){
    
    switch(m){
        
        case 'l':
            if (z_curr > 0 && isFree(x_curr, z_curr - 1)){
                z_curr --;
            }
            current_step = 'l';
            checkAndStartTimer();
            glutPostRedisplay();
            break;
        case 'r':
            if (z_curr < 14 && isFree(x_curr, z_curr + 1)){
                z_curr ++;
            }
            current_step = 'r';
            checkAndStartTimer();
            glutPostRedisplay();
            break;
        case 'f':
            if(isFree(nextX(x_curr), z_curr)){
                x_curr = nextX(x_curr);
                points ++;
                
                if(counter < 5)
                    counter ++;
                else
                    addRow(current_last_row == 19 ? 0 : current_last_row + 1);
            }
            current_step = 'f';
            checkAndStartTimer();
            glutPostRedisplay();
            printf("Points: %d\n", points);
            break;
        case 'b':
            if (x_curr > 0 && isFree(x_curr - 1, z_curr)){ // jer je na pocetku koordinata donje ivice pileta jednaka 1.08875
                x_curr --;
                counter --;
                if(jump_back_counter < 3)
                    jump_back_counter ++;
                else
                    end = 1;
            }
            current_step = 'b';
            checkAndStartTimer();
            glutPostRedisplay();
            break;
    }
    
    glutPostRedisplay();
}

static void initialize(void){
    
    initialize_fields();
    
    x_curr = 0;
    z_curr = 8;
    points = 0;
    counter = 0;
    jump_back_counter = 0;
    end = 0;
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
    
    if(animation_ongoing)
        glutTimerFunc(20, on_timer, TIMER_ID);
}

static void checkAndStartTimer(){
    
    if(!animation_ongoing){
        animation_ongoing = 1;
        glutTimerFunc(20, on_timer, TIMER_ID);
    }
}

void drawChicken(){
    
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

static void drawTree(){
    
    glPushMatrix(); // stablo
        glColor3f(0.4, 0.2, 0.1);
        glScalef(0.8, 1.5, 0.8);
        glutSolidCube(0.1);
    glPopMatrix();
    
    glPushMatrix(); // krosnja
        glColor3f(0.3, 0.5, 0.1);
        glTranslatef(0, 0.1, 0);
        glutSolidSphere(0.1, 30, 30);
    glPopMatrix();
}

static void drawCar(){
    
    glPushMatrix(); // glavni donji deo
        glColor3f(0, 0, 1);
        glScalef(1, 0.25, 0.5);
        glutSolidCube(0.2);
    glPopMatrix();
    
    glPushMatrix(); // glavni gornji deo
        glColor3f(0, 0, 0);
        glScalef(1, 0.35, 0.5);
        glTranslatef(0, 0.14, 0);
        glutSolidCube(0.13);
    glPopMatrix();
    
    glPushMatrix(); // zadnji desni tocak
        glColor3f(0.1, 0.1, 0.1);
        glTranslatef(-0.05, -0.025, 0.05);
        glScalef(1, 1, 0.1);
        glutSolidSphere(0.025, 30, 2);
    glPopMatrix();
    
    glPushMatrix(); // zadnji levi tocak
        glColor3f(0.1, 0.1, 0.1);
        glTranslatef(-0.05, -0.025, -0.05);
        glScalef(1, 1, 0.1);
        glutSolidSphere(0.025, 30, 2);
    glPopMatrix();
    
    glPushMatrix(); // prednji desni tocak
        glColor3f(0.1, 0.1, 0.1);
        glTranslatef(0.05, -0.025, 0.05);
        glScalef(1, 1, 0.1);
        glutSolidSphere(0.025, 30, 2);
    glPopMatrix();
    
    glPushMatrix(); // prednji levi tocak
        glColor3f(0.1, 0.1, 0.1);
        glTranslatef(0.05, -0.025, -0.05);
        glScalef(1, 1, 0.1);
        glutSolidSphere(0.025, 30, 2);
    glPopMatrix();
    
    glPushMatrix(); // sofersajbna
        glColor3f(0.7, 0.7, 0.7);
        glScalef(0.1, 0.25, 0.5);
        glTranslatef(0.6, 0.2, 0);
        glutSolidCube(0.11);
    glPopMatrix();
    
    glPushMatrix(); // desni prozor
        glColor3f(0.7, 0.7, 0.7);
        glScalef(1, 0.25, 0.1);
        glTranslatef(0, 0.19, -0.3);
        glutSolidCube(0.09);
    glPopMatrix();
    
    glPushMatrix(); // levi prozor
        glColor3f(0.7, 0.7, 0.7);
        glScalef(1, 0.25, 0.1);
        glTranslatef(0, 0.19, 0.3);
        glutSolidCube(0.09);
    glPopMatrix();
}

static void initialize_fields(){
    
    // rucno inicijalizujemo donje levo polje
    field[0][0].x_coordinate[0] = -1.4 - sizeX / 2;
    field[0][0].x_coordinate[1] = -1.4 + sizeX / 2;
    field[0][0].z_coordinate[0] = 1 + sizeZ / 2;
    field[0][0].z_coordinate[1] = 1 - sizeZ / 2;
    
    // popunjavamo koordinate za ostala polja u istoj koloni kao donje levo polje
    for (int i = 1; i < 20; i++){
        field[i][0].x_coordinate[0] = field[i-1][0].x_coordinate[0];
        field[i][0].x_coordinate[1] = field[i-1][0].x_coordinate[1];
        field[i][0].z_coordinate[0] = field[i-1][0].z_coordinate[0] - vector;
        field[i][0].z_coordinate[1] = field[i-1][0].z_coordinate[1] - vector;
    }
    
    for (int i = 0; i < 20; i++){
        for (int j = 1; j < 15; j++){
            field[i][j].x_coordinate[0] = field[i][j-1].x_coordinate[0] + vector;
            field[i][j].x_coordinate[1] = field[i][j-1].x_coordinate[1] + vector;
            field[i][j].z_coordinate[0] = field[i][j-1].z_coordinate[0];
            field[i][j].z_coordinate[1] = field[i][j-1].z_coordinate[1];
        }
    }
    
    for (int i = 0; i < 20; i++){
        for (int j = 0; j < 15; j++){
            field[i][j].middle_of_X = field[i][j].x_coordinate[0] + (field[i][j].x_coordinate[1] - field[i][j].x_coordinate[0]) / 2;
            field[i][j].middle_of_Z = field[i][j].z_coordinate[0] + (field[i][j].z_coordinate[1] - field[i][j].z_coordinate[0]) / 2;
        }
    }
}

static void setCar(int coordinate1, int coordinate2){
    
    glPushMatrix();
        glTranslatef(field[coordinate1][coordinate2].middle_of_X, 0, field[coordinate1][coordinate2].middle_of_Z);
        drawCar();
    glPopMatrix();
    
    field[coordinate1][coordinate2].taken = 1;
}

static void setTree(int coordinate1, int coordinate2){
    
    glPushMatrix();
        glTranslatef(field[coordinate1][coordinate2].middle_of_X, 0, field[coordinate1][coordinate2].middle_of_Z);
        drawTree();
    glPopMatrix();
    
    field[coordinate1][coordinate2].taken = 1;
}

static bool isFree(int coordinate1, int coordinate2){
    
    if(field[coordinate1][coordinate2].taken == 1)
        return 0;
    else
        return 1;
}

static void addRow(int i){
    
    current_last_row = i;
    
    // srand(time(NULL)); // za postavljanje drveca
    
    if(i != 0){
        for (int j = 0; j < 15; j++){
            field[i][j].x_coordinate[0] = field[i-1][j].x_coordinate[0];
            field[i][j].x_coordinate[1] = field[i-1][j].x_coordinate[1];
            field[i][j].z_coordinate[0] = field[i-1][j].z_coordinate[0] - vector;
            field[i][j].z_coordinate[1] = field[i-1][j].z_coordinate[1] - vector;
            field[i][j].middle_of_X = field[i][j].x_coordinate[0] + (field[i][j].x_coordinate[1] - field[i][j].x_coordinate[0]) / 2;
            field[i][j].middle_of_Z = field[i][j].z_coordinate[0] + (field[i][j].z_coordinate[1] - field[i][j].z_coordinate[0]) / 2;
        }
    }
    else{
        for (int j = 0; j < 15; j++){
            field[i][j].x_coordinate[0] = field[19][j].x_coordinate[0];
            field[i][j].x_coordinate[1] = field[19][j].x_coordinate[1];
            field[i][j].z_coordinate[0] = field[19][j].z_coordinate[0] - vector;
            field[i][j].z_coordinate[1] = field[19][j].z_coordinate[1] - vector;
            field[i][j].middle_of_X = field[i][j].x_coordinate[0] + (field[i][j].x_coordinate[1] - field[i][j].x_coordinate[0]) / 2;
            field[i][j].middle_of_Z = field[i][j].z_coordinate[0] + (field[i][j].z_coordinate[1] - field[i][j].z_coordinate[0]) / 2;
        }
    }
}

static int nextX(int currentI){
    
    return currentI == 19 ? 0 : currentI + 1;
}
