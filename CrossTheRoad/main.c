#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include "image.c"

static bool animation_ongoing = 0; 
static int animation_parameter = 0;

static float jumping_parameter = 0; // parametar koji sluzi za animiranje skoka pileta
static float jumping_active = 0; // govori nam da li je aktivno skakanje
static float camera_parameter = 0; // sluzi za pomeranje kamere pre pocetka kretanja pileta
static float cars_parameter = 0; // sluzi za pomeranje automobila

// koordinata prednje ivice pileta na pocetku z = 0.91125
// koordinata zadnje ivice pileta na pocetku z = 1.08875
// koordinata desne ivice pileta na pocetku x = 0.06375
// koordinata leve ivice pileta na pocetku x = -0.06375

static bool firstClick; // sluzi za pokretanje igrice
static int currentCarID; // indeks prvog slobodnog automobila
const static float surface_level = -0.085; // nivo gde se postavlja asfalt
const static float sizeZ = 0.1775; // duzina pileta po z osi je 0.1775
const static float sizeX = 0.1275; // sirina pileta po x osi je 0.1275
const static float vector = 0.2; // velicina za koju se pile krece
static char side; // pravac i smer u kom se pomera pile (l - left || r - right || f - forward)
static char current_step = 0; // trenutni potez koji je ucinjen - treba nam zbog rotacije pileta kad se krece u stranu
static int current_last_row; // red polja nakon kog treba dodati novi red polja
static int camera_position_z; // sluzi za pomeranje kamere
static GLuint texture_names[2]; // imena tekstura

static int x_curr; // indeks x koordinate pileta
static int z_curr; // indeks z koordinate pileta

static int score; // trenutni broj poena (uvecava se za svako kretanje unapred)
static int counter; // brojac koji treba da odluci kada pocinjemo sa dodavanjem novih polja
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
static void drawCar(float red, float green, float blue); // funkcija koja crta automobil
static void initialize_fields(); // funkcija koja inicijalizuje strukturu za polja
static void initialize_cars(); // funkcija za inicijalizaciju automobila
static void setCar(float coordinate1, float coordinate2, float red, float green, float blue); // postavlja automobil na odredjene koordinate
static void setTree(int coordinate1, int coordinate2); // postavlja drvo na odredjeno polje i postavlja vrednost polja na zauzeto
static void setAsphalt(int coordinate1, int coordinate2); // postavlja asfalt na pozicije gde se nalazi put
static bool isFree(int coordinate1, int coordinate2); // proverava da li je slobodno polje na koje pile zeli da skoci
static void addRow(int i); // funkcija koja uklanja red polja sa pocetka i dodaje novi na kraj
static int nextX(int currentX); // funkcija koja nam govori koji je sledeci indeks reda na koji mozemo da skocimo
static void updateRow(int i); // funkcija koja ubacuje drveca ili automobile na polja zadatog reda
static void checkAndStartTimer(); // funkcija koja startuje timer kojim se menja glavni animacioni parametar
static void drawBackground(); // funkcija za iscrtavanje pozadine
static void initialize_texture(void); // funkcija za inicijalizaciju teksture
static void drawGameOver(); // funkcija koja crta gameover pozadinu 
static void drawSemaphore(); // funkcija koja crta semafor na kome ce se nalaziti broj poena
static void drawScore(); // funkcija koja ispisuje skor
static void renderStrokeString(float x, float y, float z, void* font, char* string);

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
    float middle_of_X; // nosi informaciju o x koordinati sredista automobila (da bi znali gde da ga postavimo)
    float hisLimit; // nosi informaciju o x koordinati do koje se automobil krece
    float middle_of_Z; // nosi informaciju o z koordinati sredista automobila (da bi znali gde da ga postavimo)
    float x_coordinate[2]; // sadrzi informaciju o x koordinatama prednjeg i zadnjeg dela automobila (da bi znali kad tacno je udarilo pile)
    float color[3]; // sadrzi rgb vrednosti boje
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
    
    glClearColor(0, 0, 0, 0);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // kliping ravni koje ce nam sakriti pozadinu (teksturu trave) i kretanje automobila levo i desno od ivice terena
    GLdouble plane0[] = {1, 0, 0, 1.4 + sizeX};
    GLdouble plane1[] = {-1, 0, 0, 1.4 + sizeX};
    
    glEnable(GL_CLIP_PLANE0);
    glEnable(GL_CLIP_PLANE1);
    
    glClipPlane(GL_CLIP_PLANE0, plane0);
    glClipPlane(GL_CLIP_PLANE1, plane1);
    
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
    
    // odredjujemo parametar za kameru tako da se u prvih 100 frejmova pomera pozicija kamere 
    camera_parameter = animation_parameter < 100 ? animation_parameter : 100;
    camera_parameter /= 100.0;
    
    // postavljamo kameru tako da se u prvih 100 frejmova desava animacija, a da nakon toga kamera prati pile po z koordinati
    gluLookAt(0, 0.1 + 2.9 * camera_parameter, -0.1 + 3.1 * camera_parameter - 0.2 * camera_position_z,
              0, 0.07 - 0.07 * camera_parameter, field[x_curr][z_curr].middle_of_Z - 0.5 + 0.5 * camera_parameter,
              0, 1, 0);
    
    glColor3f(0, 0, 1);
    
    // postavljamo semafor na kome ce biti ispisan trenutni skor tako da se pojavi kada istekne prvih 100 frejmova (pocetna animacija), da nam ne bi kvarilo izgled animacije, i da nestane kada auto udari pile, jer ce se onda pojaviti semafor na drugom mestu
    if(!end && animation_parameter > 100){
        glPushMatrix();
            glTranslatef(1.08, 0.3, field[x_curr][z_curr].middle_of_Z + 1.04);
            drawSemaphore();
        glPopMatrix();
    }
    
    // postavljamo pozadinu (travu) ukoliko je pile i dalje u igri, a inace postavljamo semafor sa dostignutim skorom i iscrtavamo pozadinu za kraj igre kao teksturu
    glPushMatrix();
        glTranslatef(0, surface_level, field[x_curr][z_curr].middle_of_Z);
        if(end){
            drawGameOver();
            drawSemaphore();
        }
        else
            drawBackground();
    glPopMatrix();
    
    // iscrtavamo pile na trenutnoj poziciji koju translejtujemo osim x i z ose i po y osi kako bi animirali skok pileta
    glPushMatrix();
        glTranslatef(field[x_curr][z_curr].middle_of_X, sin((jumping_parameter) * PI)/25.0, field[x_curr][z_curr].middle_of_Z);
        drawChicken();
    glPopMatrix();
    
    // azuriramo parametar koji odredjuje kretanje automobila
    cars_parameter = animation_parameter % 150 / 150.0;
    
    // postavljamo automobile na redove na kojima se nalazi asfalt na desnu ili levu stranu u zavisnosti od njihove pozicije i pomeramo poziciju od pocetnog mesta do njihovog limita kretanja
    for(int i = 0; i < 20; i++){
        if(field[i][0].surface == 'a'){
            for(int k = 0; k < 20; k++){
                if(car[k].active == 1 && car[k].hisRow == i){
                    if(car[k].hisPosition == 'l')
                        setCar(car[k].middle_of_X + (car[k].hisLimit - car[k].middle_of_X) * cars_parameter, car[k].middle_of_Z, car[k].color[0], car[k].color[1], car[k].color[2]);
                    if(car[k].hisPosition == 'r')
                        setCar(car[k].middle_of_X + (car[k].hisLimit - car[k].middle_of_X) * cars_parameter, car[k].middle_of_Z, car[k].color[0], car[k].color[1], car[k].color[2]);
                }
            }
        }
        // ako je vrednost polja postavljena na zauzeto i ako se na njemu nalazi trava postavljamo drvo, inace postavljamo asfalt
        for(int j = 0; j < 15; j++){
            if(field[i][j].taken == 1 && field[i][j].surface == 'g')
                setTree(i, j);
            else if(field[i][j].surface == 'a'){
                setAsphalt(i, j);
            }
        }
    }
    for (int i = 0; i < 20; i++){ // proveravamo da li je i jedan auto udario pile i ako jeste prekidamo igru postavljanjem vrednosti indikatora prekida "end" na 1
        if(car[i].active == 1 && car[i].hisRow == x_curr && car[i].x_coordinate[0] + (car[i].hisLimit - car[i].middle_of_X) * cars_parameter < field[x_curr][z_curr].middle_of_X + sizeX / 2 && car[i].x_coordinate[1] + (car[i].hisLimit - car[i].middle_of_X) * cars_parameter > field[x_curr][z_curr].middle_of_X - sizeX / 2 && car[i].middle_of_Z == field[x_curr][z_curr].middle_of_Z) 
            end = 1;
    }
        
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
    
    // provera se vrsi da bi bilo onemoguceno kretanje dok se vrsi pocetna animacija, proveravamo firstClick da ne bi registrovalo pokretanje pocetne animacije kao pokretanje pileta (jer se animacija pokrece na jednu od strelica, a i pile se pomera na strelice) i end da ne bi mogli da pomeramo pile dalje kada se ga kola udare
    if(animation_parameter > 100 && firstClick && !end){
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
        }
   }
   checkAndStartTimer();
}

static void jumpCheck(unsigned char m){
    
    switch(m){
        
        // proveravamo da li je zeljeni potez u granicama kretanja i da li je polje slobodno, slobodno je ako se na njemu ne nalazi drvo, ako prodje proveru animiramo skok tajmer funkcijom i azuriramo current_step zbog adekvatne rotacije, kao i trenutni indeks polja na koje pile skace. Za slucaj kretanja unapred i pomeramo i kameru menjanjem camera_position_z 
        case 'l':
            if (z_curr > 0 && isFree(x_curr, z_curr - 1)){
                z_curr --;
                
                jumping_active = 1;
                glutTimerFunc(JUMPING_INTERVAL, jumping_timer, JUMPING_TIMER_ID);
            }
            current_step = 'l';
            break;
        case 'r':
            if (z_curr < 14 && isFree(x_curr, z_curr + 1)){
                z_curr ++;
                
                jumping_active = 1;
                glutTimerFunc(JUMPING_INTERVAL, jumping_timer, JUMPING_TIMER_ID);
            }
            current_step = 'r';
            break;
        case 'f':
            if(isFree(nextX(x_curr), z_curr)){
                camera_position_z ++;
                x_curr = nextX(x_curr);
                score ++;
                
                if(counter < 5)
                    counter ++;
                else // zapocinjemo dodavanje novih polja kada se pile pomeri 5 puta unapred, nakon toga za svako kretanje unapred uklanjamo jedan red sa pocetka i dodajemo ga na kraj
                    addRow(current_last_row == 19 ? 0 : current_last_row + 1);
                
                jumping_active = 1;
                glutTimerFunc(JUMPING_INTERVAL, jumping_timer, JUMPING_TIMER_ID);
            }
            current_step = 'f';
            break;
    }
    
    glutPostRedisplay();
}

static void initialize(void){
    
    initialize_texture();
    initialize_fields();
    initialize_cars();
    
    // postavljamo promenljive na pocetne vrednosti
    x_curr = 0;
    z_curr = 7;
    score = 0;
    counter = 0;
    end = 0;
    current_last_row = 19;
    camera_position_z = 0;
    currentCarID = 0;
    firstClick = 0;
    
    // postavljamo objekte na polja pocev od treceg
    for(int i = 2; i < 20; i++)
        updateRow(i);
}

static void on_reshape(int width, int height){
    
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(30, (float) width/height, 1, 1000);
    
    // prikazujemo igricu u fullscreen modu
    glutFullScreen();
}
static void on_timer(int value){
    
    if(value != TIMER_ID)
        return;
    
    // azuriramo animacioni parametar
    animation_parameter += 1;
     
    glutPostRedisplay();     
    
    if(animation_ongoing)
        glutTimerFunc(TIMER_INTERVAL, on_timer, TIMER_ID);
}

static void jumping_timer(int value){
    
    if(value != JUMPING_TIMER_ID)
        return;
    
    // azuriramo parametar koji sluzi za animiranje skoka pileta
    jumping_parameter += 0.05;
    
    if(jumping_parameter >= 1){ // ako je veci od 1 resetujemo ga na 0 i prekidamo timer funkciju
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

static void rotateChicken(char current_step){ // rotiramo pile u zavisnosti od ucinjenog koraka
    
    glRotatef(180, 0, 1, 0);
    
    if(current_step == 'l')
        glRotatef(90, 0, 1, 0);
    else if(current_step == 'r')
        glRotatef(-90, 0, 1, 0);
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

static void drawCar(float red, float green, float blue){
    
    glPushMatrix(); // glavni donji deo
        glColor3f(red, green, blue); // glavni donji deo bojimo u zavisnosti od vrednosti za boju tog automobila
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
    
    // popunjavamo koordinate za sva preostala polja
    for (int i = 0; i < 20; i++){
        for (int j = 1; j < 15; j++){
            field[i][j].x_coordinate[0] = field[i][j-1].x_coordinate[0] + vector;
            field[i][j].x_coordinate[1] = field[i][j-1].x_coordinate[1] + vector;
            field[i][j].z_coordinate[0] = field[i][j-1].z_coordinate[0];
            field[i][j].z_coordinate[1] = field[i][j-1].z_coordinate[1];
        }
    }
    
    // popunjavamo sredista koordinata polja za sva polja
    for (int i = 0; i < 20; i++){
        for (int j = 0; j < 15; j++){
            field[i][j].middle_of_X = field[i][j].x_coordinate[0] + (field[i][j].x_coordinate[1] - field[i][j].x_coordinate[0]) / 2;
            field[i][j].middle_of_Z = field[i][j].z_coordinate[0] + (field[i][j].z_coordinate[1] - field[i][j].z_coordinate[0]) / 2;
        }
    }
}

static void setCar(float coordinate1, float coordinate2, float red, float green, float blue){
    
    glPushMatrix();
        glTranslatef(coordinate1, 0, coordinate2 + 0.04); // dodajemo 0.04 da bi lepse izgledalo (inace bi se auto kretao blize jednoj ivici puta)
        drawCar(red, green, blue);
    glPopMatrix();
}

static void setTree(int coordinate1, int coordinate2){
    
    glPushMatrix();
        glTranslatef(field[coordinate1][coordinate2].middle_of_X, 0, field[coordinate1][coordinate2].middle_of_Z);
        drawTree();
    glPopMatrix();
}

static void setAsphalt(int coordinate1, int coordinate2){
    
    // crtamo asfalt kao sive skalirane kocke
    glPushMatrix();
        glColor3f(0.02, 0.02, 0.02);
        glTranslatef(field[coordinate1][coordinate2].middle_of_X, surface_level, field[coordinate1][coordinate2].middle_of_Z);
        glScalef(1, 0.05, 1);
        glutSolidCube(0.2);
    glPopMatrix();
    
    // postavljamo isprekidanu liniju na asfalt
    glPushMatrix();
        glColor3f(1, 1, 1);
        glTranslatef(field[coordinate1][coordinate2].middle_of_X, surface_level + 0.01, field[coordinate1][coordinate2].middle_of_Z);
        glScalef(1, 0, 0.05);
        glutSolidCube(0.1);
    glPopMatrix();
}

// proveravamo da li je polje slobodno, da bi znali da li da omogucimo pomeranja pileta na to polje
static bool isFree(int coordinate1, int coordinate2){
    
    return field[coordinate1][coordinate2].taken != 1;
}

static void addRow(int i){ // dodajemo novi red umesto reda i, ako je novi red onaj sa vrednoscu nula njegove koordinate postavljamo u odnosu na red sa indeksom 19, a inace sa indeksom i - 1
    
    current_last_row = i;
    
    if(i != 0){
        for (int j = 0; j < 15; j++){
            field[i][j].x_coordinate[0] = field[i-1][j].x_coordinate[0];
            field[i][j].x_coordinate[1] = field[i-1][j].x_coordinate[1];
            field[i][j].z_coordinate[0] = field[i-1][j].z_coordinate[0] - vector;
            field[i][j].z_coordinate[1] = field[i-1][j].z_coordinate[1] - vector;
            field[i][j].middle_of_X = field[i][j].x_coordinate[0] + (field[i][j].x_coordinate[1] - field[i][j].x_coordinate[0]) / 2;
            field[i][j].middle_of_Z = field[i][j].z_coordinate[0] + (field[i][j].z_coordinate[1] - field[i][j].z_coordinate[0]) / 2;
            field[i][j].taken = 0; // uklanjamo podatak o zauzetosti polja koje se prethodno cuvalo tu
            field[i][j].surface = 0; // uklanjamo podatak o tipu terena polja koje se prethodno cuvalo tu
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
            field[i][j].taken = 0; // uklanjamo podatak o zauzetosti polja koje se prethodno cuvalo tu
            field[i][j].surface = 0; // uklanjamo podatak o tipu terena polja koje se prethodno cuvalo tu
        }
    }
    
    updateRow(i); // azuriramo izgled reda
}

static void updateRow(int i){
    
    // random postavljamo na red polja travu ili asfalt
    int surfaceType = rand()%10 > 4 ? 'g' : 'a';
    
    if(surfaceType == 'g'){ // ako je trava, na prva dva i poslednja dva polja u redu postavljamo drvo
        field[i][0].taken = 1;
        field[i][1].taken = 1;
        field[i][13].taken = 1;
        field[i][14].taken = 1; 
        
        field[i][0].surface = 'g';
        field[i][1].surface = 'g';
        field[i][13].surface = 'g';
        field[i][14].surface = 'g';
        
        for (int j = 2; j < 13; j++){ // rand() funkcija odlucuje da li ce biti postavljeno drvo na ostale
            if(rand()%10 < 2){
                field[i][j].taken = 1;
                field[i][j].surface = 'g';
            }
        }
    }
    
    if(surfaceType == 'a'){ // ako je asfalt
        if(currentCarID == 19) // azuriramo identifikator trenutnog automobila kome je potrebno dodeliti asfalt
            currentCarID = 0;
        car[currentCarID].hisRow = i; // dodeljujemo mu indeks reda po kom ce se kretati
        car[currentCarID].hisPosition = rand()%10 > 4 ? 'l' : 'r'; // rand() funkcija odlucuje da li ce biti na levoj ili desnoj strani
        if(car[currentCarID].hisPosition == 'l')
            car[currentCarID].middle_of_X = field[i][0].middle_of_X - rand()%10/5.0; // postavljamo mu pocetnu lokaciju na prvo polje u redu
        else
            car[currentCarID].middle_of_X = field[i][14].middle_of_X + rand()%10/10.0; // postavljamo mu pocetnu lokaciju na poslednje polje u redu
        car[currentCarID].hisLimit = -1 * car[currentCarID].middle_of_X; // dodeljujemo mu vrednost granice do koje ce se kretati, ovim postizemo da se automobili krecu razlicitom brzinom
        car[currentCarID].middle_of_Z = field[i][0].middle_of_Z; // postavljamo mu vrednost z koordinate kojom ce se kretati
        car[currentCarID].x_coordinate[0] = car[currentCarID].middle_of_X - 0.1; // odredjujemo lokaciju zadnjeg kraja automobila na pocetku
        car[currentCarID].x_coordinate[1] = car[currentCarID].middle_of_X + 0.1; // odredjujemo lokaciju prednjeg kraja automobila na pocetku
        car[currentCarID].active = 1; // oznacavamo da je automobil aktivan (da su mu postavljene vrednosti za polja)
        car[currentCarID].color[0] = rand()%10/10.0; // rand() funkcija odlucuje rgb vrednosti za boju
        car[currentCarID].color[1] = rand()%10/10.0;
        car[currentCarID].color[2] = rand()%10/10.0;
        currentCarID ++; // uvecavamo identifikator trenutnog automobila
        
        for(int j = 0; j < 15; j++){
            field[i][j].surface = 'a'; // postavljamo vrednost tipa terena za svako polje u redu na "asfalt"
        }
    }
}

static int nextX(int currentX){
    return currentX == 19 ? 0 : currentX + 1;
}

static void initialize_cars(){ // postavljamo aktivnost na 0 da ih ne bi iscrtavali pre nego im dodelimo vrednosti za polja
    for(int i = 0; i < 20; i++){
        car[i].active = 0;
    }
}

static void checkAndStartTimer(){ // proverava i startuje glavni tajmer ako je to potrebno
    if(!animation_ongoing){
        animation_ongoing = 1;
        glutTimerFunc(TIMER_INTERVAL, on_timer, TIMER_ID);
    }
    firstClick = 1; // oznacavamo da je pokrenuta animacija
}

static void drawBackground(){
    
    glBindTexture(GL_TEXTURE_2D, texture_names[0]);
        glBegin(GL_QUADS);
                    glNormal3f(0,1,0);
                    glTexCoord2f(0, 0);
                    glVertex3f(-5, 0, 5);
                    glTexCoord2f(1, 0);
                    glVertex3f(5, 0, 5);
                    glTexCoord2f(1, 1);
                    glVertex3f(5, 0, -5);
                    glTexCoord2f(0, 1);
                    glVertex3f(-5, 0, -5);
        glEnd();
        glBindTexture(GL_TEXTURE_2D,0);
}

static void initialize_texture(void){
    
    Image* image;
    
    glEnable(GL_TEXTURE_2D);

    glTexEnvf(GL_TEXTURE_ENV,
              GL_TEXTURE_ENV_MODE,
              GL_REPLACE);

    image = image_init(0, 0);

    image_read(image, "grass.bmp"); // prva tekstura je trava

    glGenTextures(2, texture_names);

    glBindTexture(GL_TEXTURE_2D, texture_names[0]);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_WRAP_S, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_WRAP_T, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                 image->width, image->height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, image->pixels);

    glBindTexture(GL_TEXTURE_2D, 0);
    
    image_read(image, "gameover.bmp"); // druga tekstura je tekstura koja se prikazuje na kraju igre
    
    glBindTexture(GL_TEXTURE_2D, texture_names[1]);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_WRAP_S, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_WRAP_T, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                 image->width, image->height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, image->pixels);

    glBindTexture(GL_TEXTURE_2D, 0);

    image_done(image);
}

static void drawGameOver(){ // crtamo gameover teksturu
    
    glPushMatrix();
    glTranslatef(0, -surface_level, 0);
    glBindTexture(GL_TEXTURE_2D, texture_names[1]);
        glBegin(GL_QUADS);
                    glNormal3f(0, 2, 5);
                    glTexCoord2f(0, 0);
                    glVertex3f(-2.35, 0.3, 2.2);
                    glTexCoord2f(1, 0);
                    glVertex3f(2, 0.3, 2.2);
                    glTexCoord2f(1, 1);
                    glVertex3f(2, 0.3, -2.5);
                    glTexCoord2f(0, 1);
                    glVertex3f(-2.35, 0.3, -2.5);
        glEnd();
        glBindTexture(GL_TEXTURE_2D,0);
        glPopMatrix();
}

static void drawSemaphore(){ // crtamo semafor
    
    glPushMatrix();
        drawScore();
    glPopMatrix();
    
    glPushMatrix();
        glColor3f(0, 0, 0);
        glScalef(2, 0, 0.3);
        glutSolidCube(0.2);
    glPopMatrix();
}

void drawScore(){
    
    char word[10];
    sprintf(word, "SCORE: %d", score);
    
    glPushAttrib(GL_LINE_BIT); // na razlicit nacin ispisujemo skor ako je pile u igri i ako je igra zavrsena
        if(!end){
            glLineWidth(1);
            renderStrokeString(-0.18, 0, 0.004, GLUT_STROKE_ROMAN, word);
        }
        else{
            glLineWidth(3);
            renderStrokeString(-0.125, 2, 1.55, GLUT_STROKE_ROMAN, word);
        }
    glPopAttrib();
}

static void renderStrokeString(float x, float y, float z, void* font, char* string){
    
    int length;
    glDisable(GL_LIGHTING); // iskljucujemo osvetljenje da bi se lepo video skor
    
    glColor3f(1, 1, 1);
    glTranslatef(x, y, z);
    glScalef(0.0004, 0.0004, 3);
    length = strlen(string);
    for (int i = 0; i < length; i++){
        glutStrokeCharacter(font, string[i]);
    }

    glEnable(GL_LIGHTING); // ponovo ukljucujemo osvetljenje
}
