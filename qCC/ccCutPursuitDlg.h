#pragma once

// ##########################################################################
// #                                                                        #
// #                              CLOUDCOMPARE                              #
// #                                                                        #
// #  This program is free software; you can redistribute it and/or modify  #
// #  it under the terms of the GNU General Public License as published by  #
// #  the Free Software Foundation; version 2 or later of the License.      #
// #                                                                        #
// #  This program is distributed in the hope that it will be useful,       #
// #  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
// #  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          #
// #  GNU General Public License for more details.                          #
// #                                                                        #
// #          COPYRIGHT: EDF R&D / TELECOM ParisTech (ENST-TSI)             #
// #                                                                        #
// ##########################################################################

#include <ui_cutPursuitDlg.h>

//! Dialog to define Cut-Pursuit parameters
class ccCutPursuitDlg : public QDialog
    , public Ui::CutPursuitDialog
{
	Q_OBJECT

  public:
	//! Default constructor
	explicit ccCutPursuitDlg(QWidget* parent = nullptr);

	//! Returns knn parameter
	int getKNN();

	//! Returns search radius parameter
	int getKNNRadius();
	
	//! Returns regularization parameter
	int getRegularization();

	//! Returns spatial weight factor
	int getSpatialWeight();

	//! Returns cutoff parameter
	int getCutoff();

	//! Specifies whether each extracted CC should get a random color
	bool randomColors();
};
