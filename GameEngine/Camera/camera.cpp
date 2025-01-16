#include "camera.h"

Camera::Camera(glm::vec3 cameraPosition)
{
	this->cameraPosition = cameraPosition;
	this->cameraViewDirection = glm::vec3(0.0f, 0.0f, -1.0f);
	this->cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	this->cameraRight = glm::cross(cameraViewDirection, cameraUp);
	this->rotationOx = 0.0f;
	this->rotationOy = -90.0f;
}

Camera::Camera()
{
	this->cameraPosition = glm::vec3(0.0f, 0.0f, 100.0f);
	this->cameraViewDirection = glm::vec3(0.0f, 0.0f, -1.0f);
	this->cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	this->cameraRight = glm::cross(cameraViewDirection, cameraUp);
	this->rotationOx = 0.0f;
	this->rotationOy = -90.0f;
}

Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraViewDirection, glm::vec3 cameraUp)
	: isJumping(false), jumpVelocity(0.0f)
{
	this->cameraPosition = cameraPosition;
	this->cameraViewDirection = cameraViewDirection;
	this->cameraUp = cameraUp;
	this->cameraRight = glm::cross(cameraViewDirection, cameraUp);
	this->rotationOx = 0.0f;
	this->rotationOy = -90.0f;
}

Camera::~Camera()
{
}
/// 
/*
void Camera::setCameraPosition(const glm::vec3& position) {
	cameraPosition = position;
}

void Camera::keyboardMoveFront(float cameraSpeed) {
	cameraPosition += cameraViewDirection * cameraSpeed; // Move freely forward
}

void Camera::keyboardMoveBack(float cameraSpeed) {
	cameraPosition -= cameraViewDirection * cameraSpeed; // Move freely backward
}

void Camera::keyboardMoveLeft(float cameraSpeed) {
	glm::vec3 leftDirection = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), cameraViewDirection));
	cameraPosition += leftDirection * cameraSpeed; // Move freely left
}

void Camera::keyboardMoveRight(float cameraSpeed) {
	glm::vec3 rightDirection = glm::normalize(glm::cross(cameraViewDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
	cameraPosition += rightDirection * cameraSpeed; // Move freely right
}

void Camera::keyboardMoveUp(float cameraSpeed) {
	cameraPosition.y += cameraSpeed; // Move freely upward
}

void Camera::keyboardMoveDown(float cameraSpeed) {
	cameraPosition.y -= cameraSpeed; // Move freely downward
}
*/

const float GROUND_LEVEL = -20.0f; // Ground level (plane height)
const float HEAD_HEIGHT = 14.0f;    // Head height above the ground

void Camera::keyboardMoveFront(float cameraSpeed) {
	glm::vec3 nextPosition = cameraPosition + glm::vec3(cameraViewDirection.x, 0.0f, cameraViewDirection.z) * cameraSpeed;
	nextPosition.y = GROUND_LEVEL + HEAD_HEIGHT; // Keep height stable
	cameraPosition = nextPosition;
}

void Camera::keyboardMoveBack(float cameraSpeed) {
	glm::vec3 nextPosition = cameraPosition - glm::vec3(cameraViewDirection.x, 0.0f, cameraViewDirection.z) * cameraSpeed;
	nextPosition.y = GROUND_LEVEL + HEAD_HEIGHT; // Keep height stable
	cameraPosition = nextPosition;
}

void Camera::keyboardMoveLeft(float cameraSpeed) {
	glm::vec3 leftDirection = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), cameraViewDirection));
	glm::vec3 nextPosition = cameraPosition + glm::vec3(leftDirection.x, 0.0f, leftDirection.z) * cameraSpeed;
	nextPosition.y = GROUND_LEVEL + HEAD_HEIGHT; // Keep height stable
	cameraPosition = nextPosition;
}

void Camera::keyboardMoveRight(float cameraSpeed) {
	glm::vec3 rightDirection = glm::normalize(glm::cross(cameraViewDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
	glm::vec3 nextPosition = cameraPosition + glm::vec3(rightDirection.x, 0.0f, rightDirection.z) * cameraSpeed;
	nextPosition.y = GROUND_LEVEL + HEAD_HEIGHT; // Keep height stable
	cameraPosition = nextPosition;
}


void Camera::rotateOx(float angle)
{
	rotationOx += angle;

	// Constrain pitch to avoid flipping the camera
	if (rotationOx > 89.0f)
		rotationOx = 89.0f;
	if (rotationOx < -89.0f)
		rotationOx = -89.0f;

	// Calculate new camera direction based on pitch and yaw
	glm::vec3 direction;
	direction.x = cos(glm::radians(rotationOy)) * cos(glm::radians(rotationOx));
	direction.y = sin(glm::radians(rotationOx));
	direction.z = sin(glm::radians(rotationOy)) * cos(glm::radians(rotationOx));

	cameraViewDirection = glm::normalize(direction);

	// Update right and up vectors
	cameraRight = glm::normalize(glm::cross(cameraViewDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
	cameraUp = glm::normalize(glm::cross(cameraRight, cameraViewDirection));
}

void Camera::rotateOy(float angle) {
	rotationOy += angle; // Accumulate yaw

	glm::vec3 direction;
	direction.x = cos(glm::radians(rotationOy));
	direction.z = sin(glm::radians(rotationOy));
	direction.y = 0.0f; // No pitch

	cameraViewDirection = glm::normalize(direction);
	cameraRight = glm::normalize(glm::cross(cameraViewDirection, cameraUp));
}

void Camera::startJump(float jumpHeight) {
	if (!isJumping) {
		jumpVelocity = jumpHeight; // Set initial upward velocity
		isJumping = true;          // Mark the jump as active
	}
}

void Camera::updateJump(float deltaTime) {
	if (isJumping) {
		// Apply vertical movement and gravity
		cameraPosition.y += jumpVelocity * deltaTime;
		jumpVelocity -= gravity * deltaTime;

		// Stop the jump when the character lands
		if (cameraPosition.y <= GROUND_LEVEL + HEAD_HEIGHT) {
			cameraPosition.y = GROUND_LEVEL + HEAD_HEIGHT; // Reset to ground level
			isJumping = false;                             // End the jump
			jumpVelocity = 0.0f;                           // Reset velocity
		}
	}
}



void Camera::jump(float jumpHeight, float gravity, float deltaTime) {
	if (!isJumping) {
		// Start the jump by setting the initial upward velocity
		jumpVelocity = jumpHeight;
		isJumping = true;
	}

	if (isJumping) {
		// Apply vertical motion and gravity
		cameraPosition.y += jumpVelocity * deltaTime;
		jumpVelocity -= gravity * deltaTime;

		// If the character lands on the ground, stop the jump
		if (cameraPosition.y <= GROUND_LEVEL + HEAD_HEIGHT) {
			cameraPosition.y = GROUND_LEVEL + HEAD_HEIGHT; // Snap to ground level
			isJumping = false;                             // Reset jump state
			jumpVelocity = 0.0f;                           // Reset velocity
		}
	}
}



glm::mat4 Camera::getViewMatrix()
{
	return glm::lookAt(cameraPosition, cameraPosition + cameraViewDirection, cameraUp);
}

glm::vec3 Camera::getCameraPosition()
{
	return cameraPosition;
}

glm::vec3 Camera::getCameraViewDirection()
{
	return cameraViewDirection;
}

glm::vec3 Camera::getCameraUp()
{
	return cameraUp;
}

void Camera::setCameraPosition(const glm::vec3& position) {
	cameraPosition = position;
}