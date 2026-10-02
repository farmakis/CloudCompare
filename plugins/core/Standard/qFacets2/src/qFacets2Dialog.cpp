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

#include "ui_facets2Dlg.h"

#include <QSettings>

qFacets2Dialog::qFacets2Dialog( QWidget* parent )
	: QDialog( parent )
	, m_ui( new Ui::Facets2Dialog )
{
	m_ui->setupUi( this );

	loadParamsFromPersistentSettings();
}

qFacets2Dialog::~qFacets2Dialog()
{
	delete m_ui;
}

double qFacets2Dialog::getResolution() const
{
	return m_ui->doubleSpinBoxResolution->value();
}

double qFacets2Dialog::getMinPlanarity() const
{
	return m_ui->doubleSpinBoxMinPlanarity->value();
}

double qFacets2Dialog::getRegularization() const
{
	return m_ui->doubleSpinBoxRegularization->value();
}

int qFacets2Dialog::getCutoff() const
{
	return m_ui->spinBoxCutoff->value();
}

void qFacets2Dialog::loadParamsFromPersistentSettings()
{
	QSettings settings("qFacets2");

	//read parameters
	double resolution = settings.value("Resolution", m_ui->doubleSpinBoxResolution->value()).toDouble();
	double minPlanarity = settings.value("MinPlanarity", m_ui->doubleSpinBoxMinPlanarity->value()).toDouble();
	double regularization = settings.value("Regularization", m_ui->doubleSpinBoxRegularization->value()).toDouble();
	int    cutoff = settings.value("Cutoff", m_ui->spinBoxCutoff->value()).toInt();

	//apply parameters
	m_ui->doubleSpinBoxResolution->setValue(resolution);
	m_ui->doubleSpinBoxMinPlanarity->setValue(minPlanarity);
	m_ui->doubleSpinBoxRegularization->setValue(regularization);
	m_ui->spinBoxCutoff->setValue(cutoff);
}

void qFacets2Dialog::saveParamsToPersistentSettings()
{
	QSettings settings("qFacets2");
	//save parameters
	settings.setValue("Resolution", m_ui->doubleSpinBoxResolution->value());
	settings.setValue("MinPlanarity", m_ui->doubleSpinBoxMinPlanarity->value());
	settings.setValue("Regularization", m_ui->doubleSpinBoxRegularization->value());
	settings.setValue("Cutoff", m_ui->spinBoxCutoff->value());
}