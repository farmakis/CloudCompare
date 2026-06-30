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

#include "ccCutPursuitDlg.h"

#include <DgmOctree.h>

ccCutPursuitDlg::ccCutPursuitDlg(QWidget* parent /*=nullptr*/)
    : QDialog(parent, Qt::Tool)
    , Ui::CutPursuitDialog()
{
	setupUi(this);
}

int ccCutPursuitDlg::getKNN()
{
	return knnSpinBox->value();
}

int ccCutPursuitDlg::getKNNRadius()
{
	return knnRadiusSpinBox->value();
}

int ccCutPursuitDlg::getRegularization()
{
	return regularizationSpinBox->value();
}

int ccCutPursuitDlg::getSpatialWeight()
{
	return spatialWeightSpinBox->value();
}

int ccCutPursuitDlg::getCutoff()
{
	return cutoffSpinBox->value();
}

bool ccCutPursuitDlg::randomColors()
{
	return (randomColorsCheckBox->checkState() == Qt::Checked);
}
