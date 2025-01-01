#include <math.h>
#include <windows.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include <ctime>
#include <iostream>
#include <cstdlib>
#include <GL/freeglut.h>
#include <time.h>

// Constants for field and goal dimensions
const float FIELD_RADIUS = 6.0f;  // Radius of the circular field
const float GOAL_WIDTH = 1.5f;    // Width of the goal
const float GOAL_HEIGHT = 1.0f;
const float GOAL_DEPTH = 0.1f;    // Depth of the goal
const float GOAL_OFFSET = 0.5f;   // Distance from the edge of the field to the goalposts

// Window dimensions
int width = 800, height = 600;

// Variables for mouse control
int lastX = width / 2, lastY = height / 2;
bool isMousePressed = false;

// Camera angles and position
float yaw = 0.0f;
float pitch = 0.0f;
float distance = 50.0f;
float cameraX = 0.0f, cameraY = 10.0f, cameraZ = 50.0f;

// Ball properties
float ballRadius = 1.0f;
float ballPosX = 0.0f, ballPosY = ballRadius, ballPosZ = 0.0f;
float ballVelocityX = 0.0f, ballVelocityY = 0.0f, ballVelocityZ = 0.0f;
float gravity = -0.098f;

// Car dimensions
const float CAR_LENGTH = 4.0f;
const float CAR_WIDTH = 2.0f;
const float CAR_HEIGHT = 1.5f;

// Camera position
float cameraAngle = 0.0f;
float cameraDistance = 15.0f;

// Car movement properties
struct Car {
    float x;
    float z;
    float rotation;
    float speed;
    float acceleration;
    bool isAccelerating;
    bool isBraking;
    bool isTurningLeft;
    bool isTurningRight;
};

Car redCar = {5.0f, 0.0f, 0.0f, 0.0f, 0.0f, false, false, false, false};
Car blueCar = {-5.0f, 0.0f, 180.0f, 0.0f, 0.0f, false, false, false, false};

const float MAX_SPEED = 0.3f;
const float ACCELERATION = 0.01f;
const float BRAKE_FORCE = 0.02f;
const float TURN_SPEED = 3.0f;
const float FRICTION = 0.005f;

// Cloud structure
struct Cloud {
    float x, y, z;
    float speedX, speedZ;
    float size;
};

Cloud clouds[5];

void setupCamera() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (GLfloat)width / (GLfloat)height, 1.0, 800.0);

    cameraX = distance * cosf(pitch) * sinf(yaw);
    cameraY = distance * sinf(pitch);
    cameraZ = distance * cosf(pitch) * cosf(yaw);

    gluLookAt(cameraX, cameraY, cameraZ,
              0.0f, 0.0f, 0.0f,
              0.0f, 1.0f, 0.0f);
}

void mouseMotion(int x, int y) {
    if (isMousePressed) {
        int deltaX = x - lastX;
        int deltaY = y - lastY;

        yaw += deltaX * 0.01f;
        pitch -= deltaY * 0.01f;

        if (pitch > 1.5f) pitch = 1.5f;
        if (pitch < -1.5f) pitch = -1.5f;
    }
    lastX = x;
    lastY = y;
    glutPostRedisplay();
}

void mouseButton(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        isMousePressed = (state == GLUT_DOWN);
        lastX = x;
        lastY = y;
    }
}

void mouseWheel(int button, int dir, int x, int y) {
    if (dir > 0) {
        distance -= 2.0f;
        if (distance < 10.0f) distance = 10.0f;
    } else {
        distance += 2.0f;
        if (distance > 100.0f) distance = 100.0f;
    }
    glutPostRedisplay();
}

void drawCylinder(float radius, float height, int slices, int stacks) {
    GLUquadric* quad = gluNewQuadric();
    gluCylinder(quad, radius, radius, height, slices, stacks);
    gluDeleteQuadric(quad);
}

void drawHollowPipe(float outerRadius, float height) {
    glPushMatrix();
    glColor3f(1.0f, 1.0f, 1.0f);
    drawCylinder(outerRadius, height, 10, 10);
    glPopMatrix();
}

void drawField() {
    glColor3f(0.0, 0.8, 0.0);

    glBegin(GL_POLYGON);
    for (int i = 0; i < 360; i++) {
        float angle = i * 3.14159f / 180.0f;
        glVertex3f(FIELD_RADIUS * cos(angle), 0.0f, FIELD_RADIUS * sin(angle));
    }
    glEnd();

    glPushMatrix();
    glLineWidth(10.0);
    glColor3f(1.0f, 1.0f, 1.0f);
    glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, -5.9f);
    glVertex3f(0.0f, 0.0f, 5.9f);
    glEnd();
    glPopMatrix();

    glBegin(GL_LINE_STRIP);
    glLineWidth(10.0);
    float centerCircleRadius = 1.0f;
    for (int i = 0; i < 360; i++) {
        float angle = i * 3.14159f / 180.0f;
        glVertex3f(centerCircleRadius * cos(angle), 0.0f, centerCircleRadius * sin(angle));
    }
    glEnd();
}

void drawLeftGoal(float x, float z) {
    glPushMatrix();
    glColor3f(1.0f, 1.0f, 1.0f);
    glTranslatef(x, 3.0f, z);

    glPushMatrix();
    glRotatef(90.0f, 5.0f, 0.0f, 0.0f);

    glPushMatrix();
    glTranslatef(16.5f, 18.0f, 1.5f);
    drawHollowPipe(0.1f, 1.5f);
    glPopMatrix();

    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

    glPushMatrix();
    glTranslatef(13.5f, 18.0f, 1.5f);
    drawHollowPipe(0.1f, 1.5f);
    glPopMatrix();

    glPushMatrix();
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);

    glPushMatrix();
    glTranslatef(-18.0f, -1.5f, 13.5f);
    drawHollowPipe(0.1f, 3.0f);
    glPopMatrix();
}

void drawRightGoal(float x, float z) {
    glPushMatrix();
    glColor3f(1.0f, 1.0f, 1.0f);
    glTranslatef(x, 3.0f, z);

    glPushMatrix();
    glRotatef(90.0f, 5.0f, 0.0f, 0.0f);

    glPushMatrix();
    glTranslatef(16.5f, 18.0f, 1.5f);
    drawHollowPipe(0.1f, 1.5f);
    glPopMatrix();

    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

    glPushMatrix();
    glTranslatef(13.5f, 18.0f, 1.5f);
    drawHollowPipe(0.1f, 1.5f);
    glPopMatrix();

    glPushMatrix();
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);

    glPushMatrix();
    glTranslatef(-18.0f, -1.5f, 13.5f);
    drawHollowPipe(0.1f, 3.0f);
    glPopMatrix();
}

void drawWheel(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(90, 0, 1, 0);
    glColor3f(0.1f, 0.1f, 0.1f);
    glutSolidTorus(0.2f, 0.4f, 20, 20);
    glPopMatrix();
}

void drawCar(Car& car, bool isRed) {
    glPushMatrix();
    glTranslatef(car.x, 0.0f, car.z);
    glRotatef(car.rotation, 0.0f, 1.0f, 0.0f);

    // Main body
    glPushMatrix();
    if (isRed) {
        glColor3f(1.0f, 0.0f, 0.0f);
    } else {
        glColor3f(0.0f, 0.0f, 1.0f);
    }
    glScalef(4.0f, 1.5f, 2.0f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Roof
    glPushMatrix();
    glTranslatef(0.0f, 0.75f, 0.0f);
    if (isRed) {
        glColor3f(0.8f, 0.0f, 0.0f);
    } else {
        glColor3f(0.0f, 0.0f, 0.8f);
    }
    glScalef(2.4f, 0.75f, 2.0f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Windows
    glPushMatrix();
    glColor3f(0.1f, 0.1f, 0.1f);
    glTranslatef(0.0f, 0.45f, 0.0f);
    glScalef(2.0f, 0.45f, 2.1f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Wheels
    drawWheel(1.4f, -0.75f, 1.1f);
    drawWheel(1.4f, -0.75f, -1.1f);
    drawWheel(-1.4f, -0.75f, 1.1f);
    drawWheel(-1.4f, -0.75f, -1.1f);

    // Headlights
    glPushMatrix();
    glColor3f(1.0f, 1.0f, 0.0f);
    glTranslatef(1.8f, 0.0f, 0.6f);
    glutSolidSphere(0.2f, 10, 10);
    glTranslatef(0.0f, 0.0f, -1.2f);
    glutSolidSphere(0.2f, 10, 10);
    glPopMatrix();

    glPopMatrix();
}

void drawFootball(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(-x, y, z);
    glColor3f(1.0f, 1.0f, 1.0f);
    glutSolidSphere(0.25f, 20, 10);
    glPopMatrix();
}

void drawCloud(Cloud& cloud) {
    glColor4f(1.0f, 1.0f, 1.0f, 0.5f);

    for (int i = 0; i < 3; i++) {
        float offsetX = (rand() % 200 - 100) / 100.0f;
        float offsetY = (rand() % 200 - 100) / 100.0f;
        float offsetZ = (rand() % 200 - 100) / 100.0f;
        float cloudSize = cloud.size * ((rand() % 50 + 50) / 100.0f);

        glPushMatrix();
        glTranslatef(cloud.x + offsetX, cloud.y + offsetY, cloud.z + offsetZ);
        glutSolidSphere(cloudSize, 10, 10);
        glPopMatrix();
    }
}

void drawCube(float x, float y, float z, float width, float height, float depth) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(width, height, depth);

    glBegin(GL_QUADS);

    // Front face
    glColor3f(0.0, 0.0, 0.0);
    glVertex3f(-0.5, -0.5, 0.5);
    glVertex3f(0.5, -0.5, 0.5);
    glVertex3f(0.5, 0.5, 0.5);
    glVertex3f(-0.5, 0.5, 0.5);

    // Back face
    glColor3f(0.0, 0.0, 0.0);
    glVertex3f(-0.5, -0.5, -0.5);
    glVertex3f(-0.5, 0.5, -0.5);
    glVertex3f(0.5, 0.5, -0.5);
    glVertex3f(0.5, -0.5, -0.5);

    // Other faces...
    glEnd();
    glPopMatrix();
}

void updateCarPhysics(Car& car) {
    // Update acceleration
    if (car.isAccelerating) {
        car.acceleration = ACCELERATION;
    } else if (car.isBraking) {
        car.acceleration = -BRAKE_FORCE;
    } else {
        car.acceleration = 0.0f;
    }

    // Apply acceleration to speed
    car.speed += car.acceleration;

    // Apply friction
    if (car.speed > 0) {
        car.speed -= FRICTION;
    } else if (car.speed < 0) {
        car.speed += FRICTION;
    }

    // If speed is very close to 0, set it to 0
    if (fabs(car.speed) < FRICTION) {
        car.speed = 0.0f;
    }

    // Clamp speed
    if (car.speed > MAX_SPEED) car.speed = MAX_SPEED;
    if (car.speed < -MAX_SPEED/2) car.speed = -MAX_SPEED/2;

    // Update rotation
    if (car.speed != 0) {
        if (car.isTurningLeft) car.rotation += TURN_SPEED * (car.speed/MAX_SPEED);
        if (car.isTurningRight) car.rotation -= TURN_SPEED * (car.speed/MAX_SPEED);
    }

    // Convert rotation to radians for movement calculation
    float rotationRad = car.rotation * M_PI / 180.0f;

    // Update position based on speed and rotation
    car.x += car.speed * sin(rotationRad);
    car.z += car.speed * cos(rotationRad);

    // Keep cars within field bounds
    float maxDist = FIELD_RADIUS - 2.0f; // Buffer for car size
    float dist = sqrt(car.x * car.x + car.z * car.z);
    if (dist > maxDist) {
        float angle = atan2(car.x, car.z);
        car.x = maxDist * sin(angle);
        car.z = maxDist * cos(angle);
        car.speed *= 0.5f; // Reduce speed on collision
    }
}

// Keyboard function to handle regular key presses
void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        // Red car controls (WASD)
        case 'w': case 'W': redCar.isAccelerating = true; break;
        case 's': case 'S': redCar.isBraking = true; break;
        case 'a': case 'A': redCar.isTurningLeft = true; break;
        case 'd': case 'D': redCar.isTurningRight = true; break;
    }
    glutPostRedisplay();
}

// Keyboard function to handle key releases
void keyboardUp(unsigned char key, int x, int y) {
    switch(key) {
        case 'w': case 'W': redCar.isAccelerating = false; break;
        case 's': case 'S': redCar.isBraking = false; break;
        case 'a': case 'A': redCar.isTurningLeft = false; break;
        case 'd': case 'D': redCar.isTurningRight = false; break;
    }
    glutPostRedisplay();
}

// Special keyboard function for arrow keys
void specialKeys(int key, int x, int y) {
    switch(key) {
        case GLUT_KEY_UP: blueCar.isAccelerating = true; break;
        case GLUT_KEY_DOWN: blueCar.isBraking = true; break;
        case GLUT_KEY_LEFT: blueCar.isTurningLeft = true; break;
        case GLUT_KEY_RIGHT: blueCar.isTurningRight = true; break;
    }
    glutPostRedisplay();
}

// Special keyboard function for arrow key releases
void specialKeysUp(int key, int x, int y) {
    switch(key) {
        case GLUT_KEY_UP: blueCar.isAccelerating = false; break;
        case GLUT_KEY_DOWN: blueCar.isBraking = false; break;
        case GLUT_KEY_LEFT: blueCar.isTurningLeft = false; break;
        case GLUT_KEY_RIGHT: blueCar.isTurningRight = false; break;
    }
    glutPostRedisplay();
}

void update(int value) {
    updateCarPhysics(redCar);
    updateCarPhysics(blueCar);

    // Update clouds
    for (int i = 0; i < 5; i++) {
        clouds[i].x += clouds[i].speedX;
        clouds[i].z += clouds[i].speedZ;

        // Reset cloud position if it moves off screen
        if (clouds[i].x > 10.0f) clouds[i].x = -10.0f;
        if (clouds[i].x < -10.0f) clouds[i].x = 10.0f;
        if (clouds[i].z > 10.0f) clouds[i].z = -10.0f;
        if (clouds[i].z < -10.0f) clouds[i].z = 10.0f;
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0); // 60 FPS update rate
}

void display() {
    glClearColor(135.0f/255.0f, 206.0f/255.0f, 235.0f/255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setupCamera();

    // Draw the scene
    drawField();
    drawLeftGoal(-15.0f, -12.5f);
    drawRightGoal(-15.0f, -23.5f);

    // Draw seats
    drawCube(-10.0f, 0.25f, 0.0f, 1.0f, 0.5f, 15.0f);
    drawCube(-11.0f, 0.5f, 0.0f, 1.0f, 1.0f, 15.0f);
    drawCube(-12.0f, 0.75f, 0.0f, 1.0f, 1.5f, 15.0f);
    drawCube(-13.0f, 1.0f, 0.0f, 1.0f, 2.0f, 15.0f);
    drawCube(-14.0f, 1.25f, 0.0f, 1.0f, 2.5f, 15.0f);
    drawCube(-15.0f, 1.5f, 0.0f, 1.0f, 3.0f, 15.0f);

    drawFootball(0.0, 0.3, 0.0);

    // Draw cars
    drawCar(redCar, true);
    drawCar(blueCar, false);

    // Draw clouds with transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (int i = 0; i < 5; i++) {
        drawCloud(clouds[i]);
    }
    glDisable(GL_BLEND);

    glutSwapBuffers();
}

void initClouds() {
    for (int i = 0; i < 5; i++) {
        clouds[i].x = rand() % 20 - 10;
        clouds[i].y = 5 + rand() % 5;
        clouds[i].z = rand() % 20 - 10;
        clouds[i].speedX = (rand() % 3 + 1) * 0.002f;
        clouds[i].speedZ = (rand() % 3 + 1) * 0.002f;
        clouds[i].size = rand() % 3 + 2;
    }
}

void setupProjection(int w, int h) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / (double)h, 1.0, 200.0);
    glMatrixMode(GL_MODELVIEW);
}

void reshape(int w, int h) {
    width = w;
    height = h;
    glViewport(0, 0, w, h);
    setupProjection(w, h);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("3D Football Game with Moving Cars");

    glEnable(GL_DEPTH_TEST);

    // Initialize clouds
    initClouds();

    // Register callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutMouseWheelFunc(mouseWheel);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKeys);
    glutSpecialUpFunc(specialKeysUp);

    // Start the update timer
    glutTimerFunc(0, update, 0);

    glutMainLoop();
    return 0;
}
