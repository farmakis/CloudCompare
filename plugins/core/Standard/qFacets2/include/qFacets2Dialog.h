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

#pragma once

#include <QDialog>

namespace Ui
{
	class Facets2Dialog;
}

//! Dialog for the Facets2 plugin
class qFacets2Dialog : public QDialog
{
	Q_OBJECT

public:
	explicit qFacets2Dialog( QWidget* parent = nullptr );
	~qFacets2Dialog() override;

	double getResolution() const;
	double getMinPlanarity() const;
	double getRegularization() const;
	int    getCutoff() const;

	void loadParamsFromPersistentSettings();
	void saveParamsToPersistentSettings();

private:
	Ui::Facets2Dialog* m_ui;
};
