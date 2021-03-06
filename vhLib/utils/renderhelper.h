
#pragma once


#include "utlvector.h"


class IViewRender;
class IVRenderView;
class CViewSetup;
class IScaleformUI;

namespace vgui
{
	class ISurface;
	class IVGuiPaintSurface;
	typedef unsigned long HFont; 
};


// a game manager that wishes to hook into rendering
class IRenderManager
{
public:
	virtual void RenderPreHud() { }
	virtual void RenderPostHud() { }
};

namespace CRender
{
	class Rect
	{
	};
}


class CRenderHelper
{

public:
	CRenderHelper() :
		m_pViewRender( NULL ),
		m_pRenderView( NULL ),
		m_pSurface( NULL ),
		m_pPaintSurface( NULL )
	{
	}


	void Init();
	void Shutdown();

	// add and remove render hooks
	void AddRenderHook( const IRenderManager *manager );
	void RemoveRenderHook( const IRenderManager *manger );

	// engine/client render accessors
	IViewRender *ViewRender() { return m_pViewRender; }
	IVRenderView *RenderView() { return m_pRenderView; }
	// vgui accessors
	vgui::ISurface *Surface() { return m_pSurface; }
	vgui::IVGuiPaintSurface *PaintSurface() { return m_pPaintSurface; }
	// scaleform accessor
	IScaleformUI *ScaleformUI() { return m_pScaleformUI; }


	class Rect
	{

	public:
		Rect() :
			x( 0 ), y( 0 ),
			width( 0 ), height( 0 ),
			color( Color() ),
			filled( false )
		{
		}

		void Draw();

		void SetPos( int x, int y )
		{
			this->x = x;
			this->y = y;
		}
		void SetSize( int width, int height )
		{
			this->width = width;
			this->height = height;
		}
		void SetColor( Color color )
		{
			this->color = color;
		}
		void SetFilled( bool filled )
		{
			this->filled = filled;
		}


	public:
		int x, y;
		int width, height;
		Color color;
		bool filled;
	};


private:
	void PreVGui_Paint( int mode ); // IVRenderView::VGui_Paint pre hook
	void PostVGui_Paint( int mode ); // IVRenderView::VGui_Paint post hook


private:
	IViewRender *m_pViewRender;
	IVRenderView *m_pRenderView;

	vgui::ISurface *m_pSurface;
	vgui::IVGuiPaintSurface *m_pPaintSurface;

	IScaleformUI *m_pScaleformUI;

	CUtlVector<IRenderManager *> m_RenderHooks;

};

CRenderHelper &RenderHelper();
