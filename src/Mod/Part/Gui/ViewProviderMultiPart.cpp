/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
# include <BRepAlgoAPI_BooleanOperation.hxx>
# include <TopExp.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopTools_ListOfShape.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
#endif

#include "ViewProviderMultiPart.h"
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Mod/Part/App/FeaturePartBoolean.h>
#include <Mod/Part/App/FeaturePartFuse.h>
#include <Mod/Part/App/FeaturePartCommon.h>

using namespace PartGui;


ViewProviderMultiPart::ViewProviderMultiPart()
{
}


ViewProviderMultiPart::~ViewProviderMultiPart()
{
}

bool ViewProviderMultiPart::useComponentsColors(void) {
	return this->ShapeColor.getValue() == 0x000000;
}

float chooseTransparancy(float refTrans, float partTrans) {
	if (refTrans > 0) return refTrans;
	return partTrans;
}
App::Color ViewProviderMultiPart::chooseMainColor(std::vector<App::DocumentObject*> /*sources*/) {
	// if (this->useComponentsColors()) {
		// Part::Feature* objBase = dynamic_cast<Part::Feature*>(sources[0]);
		// return objBase->ShapeColor.getValue();
	// }
	return this->ShapeColor.getValue();
}


void updateColors (
	ViewProviderMultiPart *view,
	Part::FeatureMultiPart* objPart, 
	// const std::vector<Part::ShapeHistory>& hist, 
	App::Color shapeColor, 
	float curTrans ,
	bool force) {
        std::vector<App::DocumentObject*> sources = view->claimChildren();
		const std::vector<Part::ShapeHistory>& hist = objPart->History.getValues();
		printf ("sizes %ld %ld\n",hist.size(),sources.size());
        if (sources.size()==0)
            return;
		
        const TopoDS_Shape& partShape = objPart->Shape.getValue();
        TopTools_IndexedMapOfShape partMap;
        TopExp::MapShapes(partShape, TopAbs_FACE, partMap);

        std::vector<App::Color> colPart;
        colPart.resize(partMap.Extent(), shapeColor);
		
		//if (this->useComponentsColors()) {
		printf ("ShapeColor i %x\n",shapeColor.getPackedValue());
		if (shapeColor == 0x000000) {
			if (hist.size() != sources.size()) {
				// there is no history that should be calcultated while exeuting the feaure
				if (force) {
					// objPart->execute();
					return;
				} else {
					return;
				}
			} else {
				
				printf("iiiii\n");
				int index=0;
				for (std::vector<App::DocumentObject*>::iterator it = sources.begin(); it != sources.end(); ++it, ++index) {
				   Part::Feature* objBase = dynamic_cast<Part::Feature*>(*it);
					if (!objBase)
						continue;
					
					const TopoDS_Shape& baseShape = objBase->Shape.getValue();
		 
					TopTools_IndexedMapOfShape baseMap;
					TopExp::MapShapes(baseShape, TopAbs_FACE, baseMap);

					Gui::ViewProvider* vpBase = Gui::Application::Instance->getViewProvider(objBase);
					if (vpBase) {
						std::vector<App::Color> colBase = static_cast<PartGui::ViewProviderPart*>(vpBase)->DiffuseColor.getValues();
						PartGui::ViewProviderPart::applyTransparency(chooseTransparancy(curTrans, static_cast<PartGui::ViewProviderPart*>(vpBase)->Transparency.getValue()),colBase);
						if (static_cast<int>(colBase.size()) == baseMap.Extent()) {
							PartGui::ViewProviderPart::applyColor(hist[index], colBase, colPart);
						}
						else if (!colBase.empty() && colBase[0] != shapeColor) {
							colBase.resize(baseMap.Extent(), colBase[0]);
							PartGui::ViewProviderPart::applyColor(hist[index], colBase, colPart);
						}
					}
				}
			}
		}
		view->DiffuseColor.setValues(colPart);
	
}
void ViewProviderMultiPart::updateColor(const App::Color& c) {
	Part::FeatureMultiPart* objPart = static_cast<Part::FeatureMultiPart*>(getObject());
	printf("ViewProviderMultiPart::updateColor %x %s\n", c.getPackedValue(),objPart->Label.getValue());
	// printf("ViewProviderMultiPart::updateColor %s %x\n", getName(),c);
	updateColors(this, objPart,  c /*this->chooseMainColor(sources)*/, this->Transparency.getValue(), true);
}

void ViewProviderMultiPart::updateTransparency(float trans) {
	Part::FeatureMultiPart* objPart = static_cast<Part::FeatureMultiPart*>(getObject());
	updateColors(this, objPart,  this->ShapeColor.getValue() , trans, true);
}

void ViewProviderMultiPart::updateData(const App::Property* prop)
{
    PartGui::ViewProviderPart::updateData(prop);
    if (prop->getTypeId() == Part::PropertyShapeHistory::getClassTypeId()) {
        // const std::vector<Part::ShapeHistory>& hist = static_cast<const Part::PropertyShapeHistory*>
            // (prop)->getValues();
        Part::FeatureMultiPart* objPart = static_cast<Part::FeatureMultiPart*>(getObject());
		if (!objPart) 
            return;
		
		// updateColors(this, objPart, hist, this->ShapeColor.getValue() /*this->chooseMainColor(sources)*/, this->Transparency.getValue());
		updateColors(this, objPart,  this->ShapeColor.getValue() , this->Transparency.getValue(), false);

    }
    else if (prop->getTypeId().isDerivedFrom(App::PropertyLinkList::getClassTypeId())) {
        const std::vector<App::DocumentObject*>& pShapes = static_cast<const App::PropertyLinkList*>(prop)->getValues();
        for (std::vector<App::DocumentObject*>::const_iterator it = pShapes.begin(); it != pShapes.end(); ++it) {
            if (*it) {
                Gui::Application::Instance->hideViewProvider(*it);
			}
        }
	}
    else if (prop->getTypeId().isDerivedFrom(App::PropertyLink::getClassTypeId())) {
        App::DocumentObject *pBase = static_cast<const App::PropertyLink*>(prop)->getValue();
        if (pBase)
            Gui::Application::Instance->hideViewProvider(pBase);
    }
}

std::vector<App::DocumentObject*> ViewProviderMultiPart::claimChildren(void) const {
        Part::FeatureMultiPart* objPart = static_cast<Part::FeatureMultiPart*>(getObject());
		// if (!objPart) 
            // return;
		return objPart->getChildrens();
}