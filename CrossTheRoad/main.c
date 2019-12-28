#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>
#include <time.h>
#include <stdbool.h>

static bool animation_ongoing = 0; 
static int animation_parameter = 0;

static float jumping_parameter = 0; // parametar koji sluzi za animiranje skoka pileta
static float jumping_active = 0; // govori nam da li je aktivno skakanje

static float cars_parameter = 0;

// koordinata prednje ivice pileta na pocetku z = 0.91125
// koordinata zadnje ivice pileta na pocetku z = 1.08875
// koordinata desne ivice pileta na pocetku x = 0.06375
// koordinata leve ivice pileta na pocetku x = -0.06375

static int currentCarID; // indeks prvog slobodnog automobila
const static float surface_level = -0.085; // nivo gde se postavlja asfalt
const static float sizeZ = 0.1775; // duzina pileta po z osi je 0.1775
const static float sizeX = 0.1275; // sirina pileta po x osi je 0.1275
const static float vector = 0.2; // velicina za koju se pile krece
static char side; // pravac i smer u kom se pomera pile (l || r || f || b)
static char current_step = 0; // trenutni potez koji je ucinjen - treba nam zbog rotacije pileta kad se krece u stranu
static int current_last_row; // red polja nakon kog treba dodati novi red polja
static int camera_position_z; // sluzi za pomeranje kamere

static int x_curr; // indeks x koordinate pileta
static int z_curr; // indeks z koordinate pileta

static int points; // trenutni broj poena (uvecava se za svako kretanje unapred)
static int counter; // brojac koji treba da odluci kada pocinjemo sa dodavanjem novih polja
static int jump_back_counter; // brojac skokova nazad (dozvoljeno 3 skoka unazad)
static bool end; // istinitosna vrednost koja nam govori kada je kraj igre

static void on_reshape(int width, int height); 
static void on_display(void);
static void on_keyboard(unsigned char key, int x, int y);
static void on_timer(int value);
static void jumping_timer(int value); // funkcija za animiranje skoka pileta

static void on_keyboard_for_arrows(int key, int x, int y); // osluskivac za strelice
static void initialize(void);
static void jumpCheck(unsigned char m); // funkcija koja proverava da li je moguce kretanje u zadatom pravcu
static void drawChicken(); // funkcija za crtanje pileta
static void rotateChicken(char current_step); // funkcija koja rotira pile kad pri promeni pravca ili smera kretanja
static void drawTree(); // funkcija koja crta drvo
static void drawCar(); // funkcija koja crta automobil
static void initialize_fields(); // funkcija koja inicijalizuje strukturu za polja
static void initialize_cars(); // funkcija za inicijalizaciju automobila
static void setCar(float coordinate1, float coordinate2); // postavlja automobil na odredjeno polje i postavlja vrednost polja na zauzeto
static void setTree(int coordinate1, int coordinate2); // postavlja drvo na odredjeno polje i postavlja vrednost polja na zauzeto
static void setAsphalt(int coordinate1, int coordinate2); // postavlja asfalt na pozicije gde se nalazi put
static bool isFree(int coordinate1, int coordinate2); // proverava da li je slobodno polje na koje pile zeli da skoci
static void addRow(int i); // funkcija koja uklanja red polja sa pocetka i dodaje novi na kraj
static int nextX(int currentX); // funkcija koja nam govori koji je sledeci indeks reda na koji mozemo da skocimo
static void updateRow(int i); // funkcija koja ubacuje drveca ili automobile na polja zadatog reda
static void checkAndStartTimer(); // funkcija koja startuje timer kojim se menja glavni animacioni parametar

#define TIMER_ID 0
#define TIMER_INTERVAL 20

#define JUMPING_TIMER_ID 1
#define JUMPING_INTERVAL 1

#define PI 3.14159265358979323846

typedef struct field_struct{ 
    bool taken; // oznacava da li je polje zauzeto (da li se na njemu nalazi drvo)
    float x_coordinate[2]; // sadrzi informacije o levoj i desnoj granicnoj koordinati polja
    float middle_of_X; // sadrzi informaciju o x koordinati sredista polja (da bi znali gde da postavimo drvo ili pile)
    float z_coordinate[2]; // sadrzi informacije o donjoj i gornjoj granicnoj koordinati polja
    float middle_of_Z; // sadrzi informaciju o z koordinati sredista polja (da bi znali gde da postavimo drvo ili pile)
    char surface; // nosi podatak o tipu terena (trava ili asfalt)
}field_struct;

typedef struct cars{
    bool active; // oznacava da li je automobil aktivan
    int hisRow; // nosi informaciju o tome za koji red polja (prva koordinata iz field) je vezan taj automobil
    char hisPosition; // nosi informaciju o tome da li automobil dolazi sa leve ili desne strane
    float middle_of_X;
    float hisLimit;
    float middle_of_Z;
    float x_coordinate[2];
}cars;

cars car[20];
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
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float light_position[] = {-1, 1, 1, 0};
    float light_ambient[] = {.3f, .3f, .3f, 1};
    float light_diffuse[] = {.7f, .7f, .7f, 1};
    float light_specular[] = {.7f, .7f, .7f, 1};

    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    
    srand(time(NULL));
    
    initialize();
    
    glutMainLoop();
    
    return 0;
}

static void on_display(void){
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    gluLookAt(0, 3, 3 - 0.2 * camera_position_z,
              0, 0, field[x_curr][z_curr].middle_of_Z,
              0, 1, 0);
    
    glColor3f(0, 0, 1);
    
    glPushMatrix();
        glTranslatef(field[x_curr][z_curr].middle_of_X, 0 + sin((jumping_parameter) * PI)/50.0, field[x_curr][z_curr].middle_of_Z);
        drawChicken();
    glPopMatrix();
    
    cars_parameter = animation_parameter % 150 / 150.0;
    
    for(int i = 0; i < 20; i++){
        if(field[i][0].surface == 'a'){
            for(int k = 0; k < 20; k++){
                if(car[k].active == 1 && car[k].hisRow == i){
                    if(car[k].hisPosition == 'l')
                        setCar(car[k].middle_of_X + (car[k].hisLimit - car[k].middle_of_X) * cars_parameter, car[k].middle_of_Z);
                    if(car[k].hisPosition == 'r')
                        setCar(car[k].middle_of_X + (car[k].hisLimit - car[k].middle_of_X) * cars_parameter, car[k].middle_of_Z);
                }
            }
        }
        for(int j = 0; j < 15; j++){
            if(field[i][j].taken == 1 && field[i][j].surface == 'g')
                setTree(i, j);
            else if(field[i][j].surface == 'a'){
                setAsphalt(i, j);
            }
        }
    }
    for (int i = 0; i < 20; i++){
        if(car[i].active == 1 && car[i].hisRow == x_curr && car[i].x_coordinate[0] + (car[i].hisLimit - car[i].middle_of_X) * cars_parameter < field[x_curr][z_curr].middle_of_X + sizeX / 2 && car[i].x_coordinate[1] + (car[i].hisLimit - car[i].middle_of_X) * cars_parameter > field[x_curr][z_curr].middle_of_X - sizeX / 2 && car[i].middle_of_Z == field[x_curr][z_curr].middle_of_Z) 
            exit(1); // umesto ovoga obradi situaciju kada kola zgaze pile
    }
        
    glutSwapBuffers();
}

static void on_keyboard(unsigned char key, int x, int y){
    
    switch(key){
        
        case 27: // Esc
            exit(0);
            break;
        case 'g':
        case 'G':
            if(!animation_ongoing){
                animation_ongoing = 1;
                glutTimerFunc(TIMER_INTERVAL, on_timer, TIMER_ID);
            }break;
    }
}

static void on_keyboard_for_arrows(int key, int x, int y){
    
    // if(animation_parameter > 100){
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
    //}
    checkAndStartTimer();
}

static void jumpCheck(unsigned char m){
    
    switch(m){
        
        case 'l':
            if (z_curr > 0 && isFree(x_curr, z_curr - 1)){
                z_curr --;
                
                jumping_active = 1;
                glutTimerFunc(JUMPING_INTERVAL, jumping_timer, JUMPING_TIMER_ID);
            }
            current_step = 'l';
            glutPostRedisplay();
            break;
        case 'r':
            if (z_curr < 14 && isFree(x_curr, z_curr + 1)){
                z_curr ++;
                
                jumping_active = 1;
                glutTimerFunc(JUMPING_INTERVAL, jumping_timer, JUMPING_TIMER_ID);
            }
            current_step = 'r';
            glutPostRedisplay();
            break;
        case 'f':
            if(isFree(nextX(x_curr), z_curr)){
                camera_position_z ++;
                x_curr = nextX(x_curr);
                points ++;
                
                if(counter < 5)
                    counter ++;
                else
                    addRow(current_last_row == 19 ? 0 : current_last_row + 1);
                
                jumping_active = 1;
                glutTimerFunc(JUMPING_INTERVAL, jumping_timer, JUMPING_TIMER_ID);
            }
            current_step = 'f';
            glutPostRedisplay();
            printf("Points: %d\n", points);
            break;
        case 'b':
            if (x_curr > 0 && isFree(x_curr - 1, z_curr)){ // jer je na pocetku koordinata donje ivice pileta jednaka 1.08875
                x_curr --;
                counter --;
                if(jump_back_counter < 3)
                    jump_back_counter ++;
                else{ // dozvoljeno je samo 3 kretanja unazad, stoga je igra zavrsena
                    exit(1);
                }
                
                jumping_active = 1;
                glutTimerFunc(JUMPING_INTERVAL, jumping_timer, JUMPING_TIMER_ID);
            }
            current_step = 'b';
            glutPostRedisplay();
            break;
    }
    
    glutPostRedisplay();
}

static void initialize(void){
    
    initialize_fields();
    initialize_cars();
    
    x_curr = 0;
    z_curr = 7;
    points = 0;
    counter = 0;
    jump_back_counter = 0;
    end = 0;
    current_last_row = 19;
    camera_position_z = 0;
    currentCarID = 0;
    
    for(int i = 3; i < 20; i++)
        updateRow(i);
}

static void on_reshape(int width, int height){
    
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(30, (float) width/height, 1, 1000);
    
    glutFullScreen();
}
static void on_timer(int value){
    
    if(value != TIMER_ID)
        return;
    
    animation_parameter += 1;
     
    glutPostRedisplay();     
    
    if(animation_ongoing)
        glutTimerFunc(TIMER_INTERVAL, on_timer, TIMER_ID);
}

static void jumping_timer(int value){
    
    if(value != JUMPING_TIMER_ID)
        return;
    
    jumping_parameter += 0.05;
    
    if(jumping_parameter >= 1){
        jumping_parameter = 0;
        jumping_active = 0;
    }
    
    if(jumping_active)
        glutTimerFunc(JUMPING_INTERVAL, jumping_timer, JUMPING_TIMER_ID);
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

static void setCar(float coordinate1, float coordinate2){
    
    glPushMatrix();
        glTranslatef(coordinate1, 0, coordinate2);
        drawCar();
    glPopMatrix();
    
//     field[coordinate1][coordinate2].taken = 1;
}

static void setTree(int coordinate1, int coordinate2){
    
    glPushMatrix();
        glTranslatef(field[coordinate1][coordinate2].middle_of_X, 0, field[coordinate1][coordinate2].middle_of_Z);
        drawTree();
    glPopMatrix();
}

static void setAsphalt(int coordinate1, int coordinate2){
    
    glPushMatrix();
        glColor3f(0.02, 0.02, 0.02);
        glTranslatef(field[coordinate1][coordinate2].middle_of_X, surface_level, field[coordinate1][coordinate2].middle_of_Z);
        glScalef(1, 0.05, 1);
        glutSolidCube(0.2);
    glPopMatrix();
}

static bool isFree(int coordinate1, int coordinate2){
    
    if(field[coordinate1][coordinate2].taken == 1)
        return 0;
    else
        return 1;
}

static void addRow(int i){
    
    current_last_row = i;
    
    if(i != 0){
        for (int j = 0; j < 15; j++){
            field[i][j].x_coordinate[0] = field[i-1][j].x_coordinate[0];
            field[i][j].x_coordinate[1] = field[i-1][j].x_coordinate[1];
            field[i][j].z_coordinate[0] = field[i-1][j].z_coordinate[0] - vector;
            field[i][j].z_coordinate[1] = field[i-1][j].z_coordinate[1] - vector;
            field[i][j].middle_of_X = field[i][j].x_coordinate[0] + (field[i][j].x_coordinate[1] - field[i][j].x_coordinate[0]) / 2;
            field[i][j].middle_of_Z = field[i][j].z_coordinate[0] + (field[i][j].z_coordinate[1] - field[i][j].z_coordinate[0]) / 2;
            field[i][j].taken = 0;
            field[i][j].surface = 0;
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
            field[i][j].taken = 0;
            field[i][j].surface = 0;
        }
    }
    
    updateRow(i);
}

static void updateRow(int i){
    
    int surfaceType = rand()%10 > 4 ? 'g' : 'a';
    
    if(surfaceType == 'g'){
        field[i][0].taken = 1;
        field[i][1].taken = 1;
        field[i][13].taken = 1;
        field[i][14].taken = 1;
        
        field[i][0].surface = 'g';
        field[i][1].surface = 'g';
        field[i][2].surface = 'g';
        field[i][12].surface = 'g';
        field[i][13].surface = 'g';
        field[i][14].surface = 'g';
        
        for (int j = 2; j < 13; j++){
            if(rand()%10 < 2){
                field[i][j].taken = 1;
                field[i][j].surface = 'g';
            }
        }
    }
    
    if(surfaceType == 'a'){
        if(currentCarID == 19)
            currentCarID = 0;
        car[currentCarID].hisRow = i;
        car[currentCarID].hisPosition = rand()%10 > 4 ? 'l' : 'r';
        if(car[currentCarID].hisPosition == 'l')
            car[currentCarID].middle_of_X = field[i][0].middle_of_X - rand()%10/5.0;
        else
            car[currentCarID].middle_of_X = field[i][14].middle_of_X + rand()%10/10.0;
        car[currentCarID].hisLimit = -1 * car[currentCarID].middle_of_X;
        car[currentCarID].middle_of_Z = field[i][0].middle_of_Z;
        car[currentCarID].x_coordinate[0] = car[currentCarID].middle_of_X - 0.1;
        car[currentCarID].x_coordinate[1] = car[currentCarID].middle_of_X + 0.1;
        car[currentCarID].active = 1;
        currentCarID ++;
        
        for(int j = 0; j < 15; j++){
            field[i][j].surface = 'a';
        }
    }
}

static int nextX(int currentX){
    return currentX == 19 ? 0 : currentX + 1;
}

static void initialize_cars(){
    for(int i = 0; i < 20; i++){
        car[i].active = 0;
    }
}

static void checkAndStartTimer(){
    if(!animation_ongoing){
        animation_ongoing = 1;
        glutTimerFunc(TIMER_INTERVAL, on_timer, TIMER_ID);
    }
}
