#define _USE_MATH_DEFINES
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
#include <cmath>
#include <algorithm>

// Forward declarations
void setupCamera();
void drawField();
void drawLeftGoal(float x, float z);
void drawRightGoal(float x, float z);
void drawBall(float x, float y, float z);
void drawCar(float x, float z, bool isRed);
void drawHollowPipe(float outerRadius, float height);
void drawCylinder(float radius, float height, int slices, int stacks);
void reshape(int w, int h);
void mouseMotion(int x, int y);
void mouseButton(int button, int state, int x, int y);
void keyboard(unsigned char key, int x, int y);
void keyboardUp(unsigned char key, int x, int y);
void timer(int value);
void updatePhysics();
void checkCollisions();

// Constants
const float FIELD_RADIUS = 6.0f;
const float GOAL_WIDTH = 1.5f;
const float GOAL_HEIGHT = 1.0f;
const float GOAL_DEPTH = 0.1f;
const float GOAL_OFFSET = 0.5f;
const float FRICTION = 0.98f;
const float BOUNCE_FACTOR = 0.7f;
const float CAR_LENGTH = 4.0f;
const float CAR_WIDTH = 2.0f;
const float CAR_HEIGHT = 1.5f;

// Window dimensions
int width = 800, height = 600;

// Camera control
float yaw = 0.0f;
float pitch = 30.0f;
float distance = 50.0f;
float cameraX = 0.0f, cameraY = 10.0f, cameraZ = 50.0f;
bool isMousePressed = false;
int lastX, lastY;

// Ball structure
struct Ball {
    float radius = 0.25f;
    float x = 0.0f, y = 0.25f, z = 0.0f;
    float velocityX = 0.0f, velocityY = 0.0f, velocityZ = 0.0f;
    float gravity = -9.81f;
    float damping = 0.99f;
} ball;

// Car structure
struct Car {
    float x = 0.0f, z = 0.0f;
    float rotation = 0.0f;
    float speed = 0.0f;
    float acceleration = 0.0f;
    float maxSpeed = 0.5f;
    bool isRed;

    void update() {
        speed += acceleration;
        speed = std::max(-maxSpeed, std::min(maxSpeed, speed));

        float radians = rotation * M_PI / 180.0f;
        x += speed * cos(radians);
        z += speed * sin(radians);

        // Field boundary checking
        float distance = sqrt(x*x + z*z);
        if (distance > FIELD_RADIUS - 2.0f) {
            float scale = (FIELD_RADIUS - 2.0f) / distance;
            x *= scale;
            z *= scale;
            speed *= 0.5f;
        }
    }
} carRed, carBlue;

// AI Bot class
class AIBot {
public:
    float targetX = 0.0f;
    float targetZ = 0.0f;
    float decisionTimer = 0.0f;
    const float DECISION_INTERVAL = 0.5f;

    float getDistance(float x1, float z1, float x2, float z2) {
        return sqrt(pow(x2 - x1, 2) + pow(z2 - z1, 2));
    }

    float clamp(float value, float min, float max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    float sign(float value) {
        return value >= 0.0f ? 1.0f : -1.0f;
    }

    void updateAI(Car& aiCar, const Ball& ball, const Car& playerCar) {
        decisionTimer += 0.016f;

        if (decisionTimer >= DECISION_INTERVAL) {
            decisionTimer = 0.0f;
            chooseStrategy(aiCar, ball, playerCar);
        }

        moveTowardsTarget(aiCar);
    }

    void setDefensivePosition(Car& aiCar, const Ball& ball) {
        targetX = FIELD_RADIUS - 2.0f;
        targetZ = clamp(ball.z, -GOAL_WIDTH/2, GOAL_WIDTH/2);
    }

    void setAttackingTarget(Car& aiCar, const Ball& ball) {
        targetX = ball.x + ball.velocityX * 0.5f;
        targetZ = ball.z + ball.velocityZ * 0.5f;
    }

private:
    void chooseStrategy(Car& aiCar, const Ball& ball, const Car& playerCar) {
        float distanceToBall = getDistance(aiCar.x, aiCar.z, ball.x, ball.z);
        float playerDistanceToBall = getDistance(playerCar.x, playerCar.z, ball.x, ball.z);

        if (ball.x > 0) {
            setDefensivePosition(aiCar, ball);
        } else {
            setAttackingTarget(aiCar, ball);
        }
    }

    void moveTowardsTarget(Car& aiCar) {
        float targetAngle = atan2(targetZ - aiCar.z, targetX - aiCar.x) * 180.0f / M_PI;
        float angleDiff = targetAngle - aiCar.rotation;

        while (angleDiff > 180.0f) angleDiff -= 360.0f;
        while (angleDiff < -180.0f) angleDiff += 360.0f;

        if (abs(angleDiff) > 5.0f) {
            aiCar.rotation += (angleDiff > 0 ? 2.0f : -2.0f);
        }

        float distanceToTarget = getDistance(aiCar.x, aiCar.z, targetX, targetZ);
        if (distanceToTarget > 0.5f && abs(angleDiff) < 45.0f) {
            aiCar.acceleration = 0.01f;
        } else {
            aiCar.acceleration = -0.01f;
        }
    }
};

// Global AI controller
AIBot aiController;

// Camera setup
void setupCamera() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (GLfloat)width / (GLfloat)height, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float camX = distance * cos(pitch * M_PI / 180.0f) * sin(yaw * M_PI / 180.0f);
    float camY = distance * sin(pitch * M_PI / 180.0f);
    float camZ = distance * cos(pitch * M_PI / 180.0f) * cos(yaw * M_PI / 180.0f);

    gluLookAt(camX, camY, camZ, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
}

// Drawing functions
void drawCylinder(float radius, float height, int slices, int stacks) {
    GLUquadric* quad = gluNewQuadric();
    gluCylinder(quad, radius, radius, height, slices, stacks);
    gluDeleteQuadric(quad);
}

void drawHollowPipe(float outerRadius, float height) {
    glColor3f(1.0f, 1.0f, 1.0f);
    drawCylinder(outerRadius, height, 16, 1);
}

void drawField() {
    glColor3f(0.0f, 0.8f, 0.0f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 360; i++) {
        float angle = i * M_PI / 180.0f;
        glVertex3f(FIELD_RADIUS * cos(angle), 0.0f, FIELD_RADIUS * sin(angle));
    }
    glEnd();
}

void drawBall(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glColor3f(1.0f, 1.0f, 1.0f);
    glutSolidSphere(ball.radius, 16, 16);
    glPopMatrix();
}

// Main display function
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    setupCamera();
    updatePhysics();

    drawField();
    drawBall(ball.x, ball.y, ball.z);

    glPushMatrix();
    glTranslatef(carRed.x, 0.0f, carRed.z);
    glRotatef(carRed.rotation, 0.0f, 1.0f, 0.0f);
    drawCar(0.0f, 0.0f, true);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(carBlue.x, 0.0f, carBlue.z);
    glRotatef(carBlue.rotation, 0.0f, 1.0f, 0.0f);
    drawCar(0.0f, 0.0f, false);
    glPopMatrix();

    glutSwapBuffers();
}

// Update physics
void updatePhysics() {
    ball.velocityY += ball.gravity * 0.016f;
    ball.x += ball.velocityX * 0.016f;
    ball.y += ball.velocityY * 0.016f;
    ball.z += ball.velocityZ * 0.016f;

    if (ball.y < ball.radius) {
        ball.y = ball.radius;
        ball.velocityY = -ball.velocityY * BOUNCE_FACTOR;
        ball.velocityX *= FRICTION;
        ball.velocityZ *= FRICTION;
    }

    carRed.update();
    carBlue.update();
    aiController.updateAI(carBlue, ball, carRed);

    checkCollisions();
}

// Input handling
void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case 'w': carRed.acceleration = 0.01f; break;
        case 's': carRed.acceleration = -0.01f; break;
        case 'a': carRed.rotation -= 2.0f; break;
        case 'd': carRed.rotation += 2.0f; break;
        case ' ': carRed.acceleration = 0.0f; break;
    }
}

void mouseButton(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        isMousePressed = (state == GLUT_DOWN);
        lastX = x;
        lastY = y;
    }
}

void mouseMotion(int x, int y) {
    if (isMousePressed) {
        yaw += (x - lastX) * 0.5f;
        pitch = std::max(-89.0f, std::min(89.0f, pitch + (y - lastY) * 0.5f));
        lastX = x;
        lastY = y;
    }
}

void reshape(int w, int h) {
    width = w;
    height = h;
    glViewport(0, 0, w, h);
    setupCamera();
}

void timer(int value) {
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(width, height);
    glutCreateWindow("Football Game with AI");

    glEnable(GL_DEPTH_TEST);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutTimerFunc(16, timer, 0);

    carRed.isRed = true;
    carBlue.isRed = false;
    carRed.x = -3.0f;
    carBlue.x = 3.0f;

    glutMainLoop();
    return 0;
}
