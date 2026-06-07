#define GL_SILENCE_DEPRECATION

#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/glut.h>
#include <cstdlib>
#include <cmath>

const float PI = 3.141592f;

float gXAngle = 10.0f;
float gYAngle = 0.0f;
float cameraDistance = 4.8f;

int moveX = 0;
int moveY = 0;
bool isDragging = false;

GLuint headTexture;
GLuint bodyTexture;
const int TEX_W = 512;
const int TEX_H = 512;
unsigned char headTex[TEX_H][TEX_W][3];

void setColor(float r, float g, float b)
{
	glColor3f(r, g, b);
}

float clampValue(float value, float minValue, float maxValue)
{
	if (value < minValue) return minValue;
	if (value > maxValue) return maxValue;
	return value;
}

void drawEllipsoid(float rx, float ry, float rz)
{
	glPushMatrix();
	glScalef(rx, ry, rz);
	glutSolidSphere(1.0, 40, 40);
	glPopMatrix();
}

void drawCylinderY(float radius, float height)
{
	GLUquadric* quad = gluNewQuadric();

	glPushMatrix();
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

	gluDisk(quad, 0.0, radius, 32, 1);
	gluCylinder(quad, radius, radius, height, 32, 1);

	glTranslatef(0.0f, 0.0f, height);
	gluDisk(quad, 0.0, radius, 32, 1);

	glPopMatrix();

	gluDeleteQuadric(quad);
}

void putPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
	if (x < 0 || x >= TEX_W || y < 0 || y >= TEX_H) return;

	headTex[y][x][0] = r;
	headTex[y][x][1] = g;
	headTex[y][x][2] = b;
}

void drawTextureOval(
	int cx, int cy,
	int rx, int ry,
	unsigned char r, unsigned char g, unsigned char b
)
{
	for (int y = cy - ry; y <= cy + ry; y++) {
		for (int x = cx - rx; x <= cx + rx; x++) {
			float dx = (float)(x - cx) / rx;
			float dy = (float)(y - cy) / ry;

			if (dx * dx + dy * dy <= 1.0f) {
				putPixel(x, y, r, g, b);
			}
		}
	}
}

void drawTextureRotatedOval(
	int cx, int cy,
	int rx, int ry,
	float angleDeg,
	unsigned char r, unsigned char g, unsigned char b
)
{
	float a = angleDeg * PI / 180.0f;
	float ca = cos(a);
	float sa = sin(a);

	int bound = rx + ry + 4;

	for (int y = cy - bound; y <= cy + bound; y++) {
		for (int x = cx - bound; x <= cx + bound; x++) {
			float dx = (float)(x - cx);
			float dy = (float)(y - cy);

			float localX = dx * ca + dy * sa;
			float localY = -dx * sa + dy * ca;

			float nx = localX / rx;
			float ny = localY / ry;

			if (nx * nx + ny * ny <= 1.0f) {
				int wrappedX = x;
				while (wrappedX < 0) wrappedX += TEX_W;
				while (wrappedX >= TEX_W) wrappedX -= TEX_W;
				putPixel(wrappedX, y, r, g, b);
			}
		}
	}
}

void buildHeadTexture()
{
	for (int y = 0; y < TEX_H; y++) {
		for (int x = 0; x < TEX_W; x++) {
			headTex[y][x][0] = 250;
			headTex[y][x][1] = 246;
			headTex[y][x][2] = 224;
		}
	}

	drawTextureRotatedOval(213, 258, 40, 62, 10.0f, 246, 218, 175);
	drawTextureRotatedOval(392, 220, 82, 98, -12.0f, 246, 218, 175);
	drawTextureRotatedOval(365, 430, 110, 75, -4.0f, 241, 145, 35);
	drawTextureRotatedOval(540, 295, 30, 60, -5.0f, 246, 218, 175);

	glGenTextures(1, &headTexture);
	glBindTexture(GL_TEXTURE_2D, headTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEX_W, TEX_H, 0, GL_RGB, GL_UNSIGNED_BYTE, headTex);

	// 몸통용 단색 텍스처 (얼굴 베이스와 동일한 색상)
	unsigned char bodyColor[3] = { 250, 246, 224 };
	glGenTextures(1, &bodyTexture);
	glBindTexture(GL_TEXTURE_2D, bodyTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, bodyColor);
}

void drawTexturedHead()
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, headTexture);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glColor3f(1.0f, 1.0f, 1.0f);

	const int slices = 90;
	const int stacks = 45;

	const float rx = 0.82f;
	const float ry = 0.78f;
	const float rz = 0.70f;

	for (int i = 0; i < stacks; i++) {
		float phi1 = -PI / 2.0f + PI * i / stacks;
		float phi2 = -PI / 2.0f + PI * (i + 1) / stacks;

		float v1 = (float)i / stacks;
		float v2 = (float)(i + 1) / stacks;

		glBegin(GL_QUAD_STRIP);

		for (int j = 0; j <= slices; j++) {
			float u = (float)j / slices;
			float theta = 2.0f * PI * (u - 0.5f);

			float x1 = rx * cos(phi1) * sin(theta);
			float y1 = ry * sin(phi1);
			float z1 = rz * cos(phi1) * cos(theta);

			float x2 = rx * cos(phi2) * sin(theta);
			float y2 = ry * sin(phi2);
			float z2 = rz * cos(phi2) * cos(theta);

			glNormal3f(x1 / rx, y1 / ry, z1 / rz);
			glTexCoord2f(u, v1);
			glVertex3f(x1, y1, z1);

			glNormal3f(x2 / rx, y2 / ry, z2 / rz);
			glTexCoord2f(u, v2);
			glVertex3f(x2, y2, z2);
		}

		glEnd();
	}

	glDisable(GL_TEXTURE_2D);
}

void drawSmoothBody()
{
	const int slices = 40;
	const int rings = 8;

	float y[rings] = {
		 0.34f,  0.28f,  0.18f,  0.05f,
		-0.10f, -0.22f, -0.32f, -0.38f
	};

	float r[rings] = {
		0.23f, 0.27f, 0.32f, 0.36f,
		0.36f, 0.33f, 0.25f, 0.10f
	};

	// 각 링마다 스무스 노말 사전 계산
	float nx[rings] = {}, ny[rings] = {};

	for (int i = 0; i < rings; i++) {
		float ax = 0, ay = 0;
		if (i > 0) {
			float dy = y[i] - y[i - 1], dr = r[i] - r[i - 1];
			float len = sqrt(dy * dy + dr * dr);
			ax += -dy / len;
			ay += dr / len;
		}
		if (i < rings - 1) {
			float dy = y[i + 1] - y[i], dr = r[i + 1] - r[i];
			float len = sqrt(dy * dy + dr * dr);
			ax += -dy / len;
			ay += dr / len;
		}
		float len = sqrt(ax * ax + ay * ay);
		nx[i] = ax / len;
		ny[i] = ay / len;
	}

	for (int i = 0; i < rings - 1; i++) {
		glBegin(GL_QUAD_STRIP);
		for (int j = 0; j <= slices; j++) {
			float theta = 2.0f * PI * j / slices;
			float c = cos(theta), s = sin(theta);

			glNormal3f(c * nx[i], ny[i], s * nx[i]);
			glVertex3f(r[i] * c, y[i], r[i] * s);

			glNormal3f(c * nx[i + 1], ny[i + 1], s * nx[i + 1]);
			glVertex3f(r[i + 1] * c, y[i + 1], r[i + 1] * s);
		}
		glEnd();
	}

	glBegin(GL_TRIANGLE_FAN);
	glNormal3f(0.0f, 1.0f, 0.0f);
	glVertex3f(0.0f, y[0], 0.0f);
	for (int j = 0; j <= slices; j++) {
		float theta = 2.0f * PI * j / slices;
		glVertex3f(r[0] * cos(theta), y[0], r[0] * sin(theta));
	}
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	glNormal3f(0.0f, -1.0f, 0.0f);
	glVertex3f(0.0f, y[rings - 1], 0.0f);
	for (int j = 0; j <= slices; j++) {
		float theta = 2.0f * PI * j / slices;
		glVertex3f(r[rings - 1] * cos(theta), y[rings - 1], r[rings - 1] * sin(theta));
	}
	glEnd();
}

void drawCurvedWhisker(
	float x0, float y0, float z0,
	float cx, float cy, float cz,
	float x1, float y1, float z1
)
{
	glBegin(GL_LINE_STRIP);

	for (int i = 0; i <= 32; i++) {
		float t = i / 32.0f;
		float u = 1.0f - t;

		float x = u * u * x0 + 2.0f * u * t * cx + t * t * x1;
		float y = u * u * y0 + 2.0f * u * t * cy + t * t * y1;
		float z = u * u * z0 + 2.0f * u * t * cz + t * t * z1;

		glVertex3f(x, y, z);
	}

	glEnd();
}

void myLight()
{
	GLfloat ambient[] = { 0.35f, 0.35f, 0.35f, 1.0f };
	GLfloat diffuse[] = { 0.95f, 0.95f, 0.95f, 1.0f };
	GLfloat position[] = { 3.0f, 5.0f, 6.0f, 1.0f };

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, position);

	glShadeModel(GL_SMOOTH);
}

void drawGround()
{
	glDisable(GL_LIGHTING);

	setColor(0.96f, 0.90f, 0.93f);

	glBegin(GL_QUADS);
	glVertex3f(-4.0f, -1.35f, -4.0f);
	glVertex3f(4.0f, -1.35f, -4.0f);
	glVertex3f(4.0f, -1.35f, 4.0f);
	glVertex3f(-4.0f, -1.35f, 4.0f);
	glEnd();

	setColor(0.72f, 0.68f, 0.70f);

	glPushMatrix();
	glTranslatef(0.0f, -1.30f, 0.0f);
	drawEllipsoid(0.85f, 0.03f, 0.55f);
	glPopMatrix();

	glEnable(GL_LIGHTING);
}

void drawHead()
{
	glPushMatrix();
	drawTexturedHead();
	glPopMatrix();

	// right ear - outer orange
	glPushMatrix();
	glTranslatef(0.57f, 0.46f, -0.02f);
	glRotatef(-35.0f, 0.0f, 0.0f, 1.0f);
	glRotatef(10.0f, 1.0f, 0.0f, 0.0f);
	setColor(235.0f / 255.0f, 142.0f / 255.0f, 36.0f / 255.0f);
	drawEllipsoid(0.24f, 0.40f, 0.12f);
	glPopMatrix();

	// right ear - inner cream
	glPushMatrix();
	glTranslatef(0.57f, 0.44f, 0.05f);
	glRotatef(-35.0f, 0.0f, 0.0f, 1.0f);
	glRotatef(10.0f, 1.0f, 0.0f, 0.0f);
	setColor(246.0f / 255.0f, 240.0f / 255.0f, 218.0f / 255.0f);
	drawEllipsoid(0.17f, 0.31f, 0.06f);
	glPopMatrix();

	// left ear - outer orange
	glPushMatrix();
	glTranslatef(-0.57f, 0.46f, -0.02f);
	glRotatef(35.0f, 0.0f, 0.0f, 1.0f);
	glRotatef(10.0f, 1.0f, 0.0f, 0.0f);
	setColor(235.0f / 255.0f, 142.0f / 255.0f, 36.0f / 255.0f);
	drawEllipsoid(0.24f, 0.42f, 0.12f);
	glPopMatrix();

	// left ear - inner cream
	glPushMatrix();
	glTranslatef(-0.57f, 0.44f, 0.05f);
	glRotatef(35.0f, 0.0f, 0.0f, 1.0f);
	glRotatef(10.0f, 1.0f, 0.0f, 0.0f);
	setColor(246.0f / 255.0f, 240.0f / 255.0f, 218.0f / 255.0f);
	drawEllipsoid(0.17f, 0.31f, 0.06f);
	glPopMatrix();

	// eyes
	glPushMatrix();
	glTranslatef(-0.30f, -0.02f, 0.73f);
	glRotatef(-25.0f, 0.0f, 0.0f, 1.0f);
	setColor(0.0f, 0.0f, 0.0f);
	drawEllipsoid(0.22f, 0.16f, 0.04f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.30f, -0.02f, 0.73f);
	glRotatef(25.0f, 0.0f, 0.0f, 1.0f);
	setColor(0.0f, 0.0f, 0.0f);
	drawEllipsoid(0.22f, 0.16f, 0.04f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-0.30f, -0.02f, 0.75f);
	glRotatef(-25.0f, 0.0f, 0.0f, 1.0f);
	setColor(1.0f, 1.0f, 1.0f);
	drawEllipsoid(0.20f, 0.14f, 0.05f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.30f, -0.02f, 0.75f);
	glRotatef(25.0f, 0.0f, 0.0f, 1.0f);
	setColor(1.0f, 1.0f, 1.0f);
	drawEllipsoid(0.20f, 0.14f, 0.05f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-0.25f, 0.01f, 0.81f);
	glRotatef(-25.0f, 0.0f, 0.0f, 1.0f);
	setColor(0.0f, 0.0f, 0.0f);
	drawEllipsoid(0.15f, 0.13f, 0.04f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.27f, 0.01f, 0.81f);
	glRotatef(25.0f, 0.0f, 0.0f, 1.0f);
	setColor(0.0f, 0.0f, 0.0f);
	drawEllipsoid(0.15f, 0.13f, 0.04f);
	glPopMatrix();

	// nose
	glPushMatrix();
	glTranslatef(0.0f, -0.22f, 0.80f);
	setColor(0.0f, 0.0f, 0.0f);
	drawEllipsoid(0.07f, 0.055f, 0.045f);
	glPopMatrix();

	glDisable(GL_LIGHTING);
	setColor(0.0f, 0.0f, 0.0f);

	// eyebrow
	glLineWidth(5.0f);
	glBegin(GL_LINES);
	glVertex3f(0.05f, 0.14f, 0.81f);
	glVertex3f(0.18f, 0.19f, 0.81f);
	glEnd();

	// mouth
	glBegin(GL_LINES);
	glVertex3f(0.0f, -0.22f, 0.82f);
	glVertex3f(0.0f, -0.29f, 0.82f);
	glEnd();

	drawCurvedWhisker(0.0f, -0.29f, 0.82f, -0.05f, -0.34f, 0.82f, -0.11f, -0.29f, 0.82f);
	drawCurvedWhisker(0.0f, -0.29f, 0.82f, 0.05f, -0.34f, 0.82f, 0.11f, -0.29f, 0.82f);

	glLineWidth(3.0f);

	drawCurvedWhisker(-0.42f, -0.07f, 0.60f, -0.70f, -0.09f, 0.62f, -1.00f, 0.02f, 0.64f);
	drawCurvedWhisker(-0.42f, -0.15f, 0.60f, -0.70f, -0.09f, 0.62f, -1.00f, -0.27f, 0.64f);
	drawCurvedWhisker(0.42f, -0.07f, 0.60f, 0.70f, -0.09f, 0.62f, 1.00f, 0.02f, 0.64f);
	drawCurvedWhisker(0.42f, -0.15f, 0.60f, 0.70f, -0.09f, 0.62f, 1.00f, -0.27f, 0.64f);

	glLineWidth(1.0f);
	glEnable(GL_LIGHTING);
}

void drawBody()
{
	const float CREAM_R = 246.0f / 255.0f;
	const float CREAM_G = 218.0f / 255.0f;
	const float CREAM_B = 175.0f / 255.0f;

	const float ORANGE_R = 241.0f / 255.0f;
	const float ORANGE_G = 145.0f / 255.0f;
	const float ORANGE_B = 35.0f / 255.0f;

	// body texture pipeline
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, bodyTexture);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glColor3f(1.0f, 1.0f, 1.0f);

	glPushMatrix();
	glTranslatef(0.0f, -0.03f, 0.0f);
	drawSmoothBody();
	glPopMatrix();

	float armRadius = 0.095f;
	float armLength = 0.50f;

	glPushMatrix();
	glTranslatef(0.16f, 0.32f, 0.08f);
	glRotatef(-140.0f, 0.0f, 0.0f, 1.0f);
	drawCylinderY(armRadius, armLength);
	glTranslatef(0.0f, armLength, 0.0f);
	glDisable(GL_TEXTURE_2D);
	setColor(ORANGE_R, ORANGE_G, ORANGE_B);
	glutSolidSphere(armRadius, 32, 32);
	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-0.16f, 0.32f, 0.08f);
	glRotatef(140.0f, 0.0f, 0.0f, 1.0f);
	drawCylinderY(armRadius, armLength);
	glTranslatef(0.0f, armLength, 0.0f);
	glDisable(GL_TEXTURE_2D);
	setColor(ORANGE_R, ORANGE_G, ORANGE_B);
	glutSolidSphere(armRadius, 32, 32);
	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 1.0f);
	glPopMatrix();

	float legRadius = 0.10f;
	float legLength = 0.30f;

	glPushMatrix();
	glTranslatef(-0.13f, -0.25f, 0.03f);
	glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
	drawCylinderY(legRadius, legLength);
	glTranslatef(0.0f, legLength, 0.0f);
	glDisable(GL_TEXTURE_2D);
	setColor(ORANGE_R, ORANGE_G, ORANGE_B);
	glutSolidSphere(legRadius, 32, 32);
	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 1.0f);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.13f, -0.25f, 0.03f);
	glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
	drawCylinderY(legRadius, legLength);
	glTranslatef(0.0f, legLength, 0.0f);
	glDisable(GL_TEXTURE_2D);
	setColor(ORANGE_R, ORANGE_G, ORANGE_B);
	glutSolidSphere(legRadius, 32, 32);
	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 1.0f);
	glPopMatrix();

	glDisable(GL_TEXTURE_2D);

	// tail
	glPushMatrix();
	glTranslatef(0.32f, -0.12f, -0.22f);
	glRotatef(-70.0f, 0.0f, 0.0f, 1.0f);
	glRotatef(-20.0f, 1.0f, 0.0f, 0.0f);

	for (int i = 0; i < 5; i++) {
		if (i % 2 == 0) setColor(ORANGE_R, ORANGE_G, ORANGE_B);
		else setColor(1.0f, 1.0f, 1.0f);

		drawCylinderY(0.06f, 0.13f);

		glPushMatrix();
		glTranslatef(0.0f, 0.13f, 0.0f);
		glutSolidSphere(0.06f, 20, 20);
		glPopMatrix();

		glTranslatef(0.0f, 0.13f, 0.0f);
		glRotatef(-5.0f, 0.0f, 0.0f, 1.0f);
	}

	glPopMatrix();

	// belly flower - 4 petals
	setColor(ORANGE_R, ORANGE_G, ORANGE_B);

	float petalAngleOffset = 0.0f;
	int petals = 4;
	float petalDist = 0.09f;

	for (int i = 0; i < petals; i++) {
		float angle = petalAngleOffset + 2.0f * PI * i / petals;
		float px = petalDist * sin(angle);
		float py = petalDist * cos(angle);

		glPushMatrix();
		glTranslatef(px, py - 0.05f, 0.38f);
		glRotatef(angle * 180.0f / PI, 0.0f, 0.0f, 1.0f);
		drawEllipsoid(0.075f, 0.075f, 0.03f);
		glPopMatrix();
	}

	// flower center
	glPushMatrix();
	glTranslatef(0.0f, -0.05f, 0.38f);
	drawEllipsoid(0.065f, 0.065f, 0.03f);
	glPopMatrix();
}

void drawCatppi()
{
	glPushMatrix();
	glTranslatef(0.0f, -0.42f, 0.0f);
	drawBody();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f, 0.50f, 0.0f);
	drawHead();
	glPopMatrix();
}

void myDisplay()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gluLookAt(
		0.0f, 0.8f, cameraDistance,
		0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f
	);

	drawGround();

	glPushMatrix();
	glRotatef(gYAngle, 0.0f, 1.0f, 0.0f);
	glRotatef(gXAngle, 1.0f, 0.0f, 0.0f);
	drawCatppi();
	glPopMatrix();

	glutSwapBuffers();
}

void reshape(int w, int h)
{
	if (h == 0) h = 1;

	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(45.0, (double)w / (double)h, 0.1, 100.0);

	glMatrixMode(GL_MODELVIEW);
}

void myKeyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case 'a':
	case 'A':
		cameraDistance -= 0.25f;
		cameraDistance = clampValue(cameraDistance, 3.0f, 10.0f);
		break;

	case 'z':
	case 'Z':
		cameraDistance += 0.25f;
		cameraDistance = clampValue(cameraDistance, 3.0f, 10.0f);
		break;

	case 'q':
	case 'Q':
	case 27:
		std::exit(0);
		break;
	}

	glutPostRedisplay();
}

void myMouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			isDragging = true;
			moveX = x;
			moveY = y;
		}
		else {
			isDragging = false;
		}
	}
}

void myMouseMotion(int x, int y)
{
	if (!isDragging) return;

	int dx = x - moveX;
	int dy = y - moveY;

	gYAngle += dx * 0.5f;
	gXAngle += dy * 0.5f;

	moveX = x;
	moveY = y;

	glutPostRedisplay();
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(800, 700);
	glutCreateWindow("Catppi Texture Mapping");

	glClearColor(1.0f, 0.96f, 0.98f, 1.0f);

	myLight();
	buildHeadTexture();

	glutDisplayFunc(myDisplay);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(myKeyboard);
	glutMouseFunc(myMouse);
	glutMotionFunc(myMouseMotion);

	glutMainLoop();

	return 0;
}
