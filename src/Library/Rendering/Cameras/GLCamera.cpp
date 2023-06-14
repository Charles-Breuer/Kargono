#include "Cameras.h"
#include "../../Library.h"

/*============================================================================================================================================================================================
 *============================================================================================================================================================================================
 * GLCamera Class
 *============================================================================================================================================================================================
 *============================================================================================================================================================================================*/

 /*===========================================================================e3=================================================================================================================
  * External Functionality
  *============================================================================================================================================================================================*/


void GLCamera::processProjection(glm::mat4 &view, glm::mat4 &projection)
{
    view = glm::lookAt(orientation.cameraPosition, orientation.cameraPosition + orientation.cameraFront, orientation.cameraUp);
    if (this->projection == PERSPECTIVE)
    {
        projection = glm::perspective(glm::radians(fov), (float)Resources::currentRenderer->screenDimension.x / (float)Resources::currentRenderer->screenDimension.y, 0.1f, 100.0f);
    }
    else if (this->projection == ORTHOGRAPHIC)
    {
        projection = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, 0.1f, 100.0f);
    }
    else
    {
        std::cout << "ERROR::PROJECTION::PROJECTION_ENUM_INCORRECT" << std::endl;
        throw std::runtime_error("Check Logs");
    }
}
