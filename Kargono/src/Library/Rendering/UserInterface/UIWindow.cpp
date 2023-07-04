#include "Kargono/kgpch.h"
#include "../../Includes.h"
#include "WindowFunctions.h"
#include "../../Library.h"
#include "UIFunctions.h"


/*============================================================================================================================================================================================
 *============================================================================================================================================================================================
 * UIWindow Class
 *============================================================================================================================================================================================
 *============================================================================================================================================================================================*/


void UIWindow::renderWindow()
{
	switch (type)
	{
	case (MAIN_MENU):
		renderMainMenu();
		break;
	case (DEMO_WINDOW):
		renderDemo();
		break;
	case (DEV_TOOLS):
		renderDevTool();
		break;
	}

}


void UIWindow::closeChildren()
{
	for (int i = 0; i < children.size(); i++)
	{
		children.at(i)->closeChildren();
		
	}
	isRendering = false;
}


