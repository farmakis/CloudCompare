//##########################################################################
//#                                                                        #
//#                CLOUDCOMPARE PLUGIN: qFacets2                          #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 of the License.               #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#                             COPYRIGHT: Ioannis Farmakis                #
//#                                                                        #
//##########################################################################

#include "qFacets2Dialog.h"

//CCPluginAPI
#include <ccMainAppInterface.h>

//Qt
#include <QMainWindow>


qFacets2Dialog::qFacets2Dialog( ccMainAppInterface* app )
	: QDialog( app ? app->getMainWindow() : nullptr )
	, Ui::Facets2Dialog()
	, m_app( app )
{
	setupUi( this );
	
	loadParamsFromPersistentSettings();
}

double qFacets2Dialog::getResolution() const
{
	return doubleSpinBoxResolution->value();
}

double qFacets2Dialog::getMinPlanarity() const
{
	return doubleSpinBoxMinPlanarity->value();
}

double qFacets2Dialog::getRegularization() const
{
	return doubleSpinBoxRegularization->value();
}

int qFacets2Dialog::getCutoff() const
{
	return spinBoxCutoff->value();
}

void qFacets2Dialog::loadParamsFromPersistentSettings()
{
	QSettings settings("qFacets2");

	//read parameters
	double resolution = settings.value("Resolution", doubleSpinBoxResolution->value()).toDouble();
	double minPlanarity = settings.value("MinPlanarity", doubleSpinBoxMinPlanarity->value()).toDouble();
	double regularization = settings.value("Regularization", doubleSpinBoxRegularization->value()).toDouble();
	int    cutoff = settings.value("Cutoff", spinBoxCutoff->value()).toInt();

	//apply parameters
	doubleSpinBoxResolution->setValue(resolution);
	doubleSpinBoxMinPlanarity->setValue(minPlanarity);
	doubleSpinBoxRegularization->setValue(regularization);
	spinBoxCutoff->setValue(cutoff);
}

void qFacets2Dialog::saveParamsToPersistentSettings()
{
	QSettings settings("qFacets2");
	//save parameters
	settings.setValue("Resolution", doubleSpinBoxResolution->value());
	settings.setValue("MinPlanarity", doubleSpinBoxMinPlanarity->value());
	settings.setValue("Regularization", doubleSpinBoxRegularization->value());
	settings.setValue("Cutoff", spinBoxCutoff->value());
}