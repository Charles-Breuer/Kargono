#include "Input.h"

/*============================================================================================================================================================================================
 *============================================================================================================================================================================================
 * GLButton Class
 *============================================================================================================================================================================================
 *============================================================================================================================================================================================*/

 /*============================================================================================================================================================================================
  * Default Constructor
  *============================================================================================================================================================================================*/

GLButton::GLButton()
{
    previousState = GLFW_RELEASE;
    glfwValue = -1;
    function = FunctionReferences::NA;
}