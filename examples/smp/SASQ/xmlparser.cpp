// --------------------------------------------
// Copyright KAPSARC. Open source MIT License.
// --------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2015 King Abdullah Petroleum Studies and Research Center
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// -------------------------------------------------

#include "xmlparser.h"
#include <QDebug>

Xmlparser::Xmlparser(QString homeDir)
{
    homeDirectory=homeDir;
}

void Xmlparser::openXmlFile(QString xmlFilePath)
{
    QFile* xmlFile = new QFile(xmlFilePath);

    if(xmlFile->open(QIODevice::ReadOnly | QIODevice::Text))
    {
        xmlFileName=xmlFilePath;
        emit openXMLStatus(true);
    }
    else
        emit openXMLStatus(false);
}

void Xmlparser::readXmlFile()
{
    xmlModel = new QStandardItemModel;

    dimensionNames.clear();
    modelParametes.clear();
    actorsList.clear();
    idealAdjustmentList.clear();

    QFile* xmlFile = new QFile(xmlFileName);

    xmlFile->open(QIODevice::ReadOnly | QIODevice::Text);

    xmlReader.setDevice(xmlFile);

    //Reading from the file
    while(!xmlReader.atEnd() && !xmlReader.hasError())
    {
        QXmlStreamReader::TokenType token =xmlReader.readNext();
        if(token == QXmlStreamReader::StartDocument)
            continue;

        if (xmlReader.isStartElement())
        {
            QString tokenName = xmlReader.name().toString();
            if (tokenName == "name")
            {
                name = xmlReader.readElementText();
                //                qDebug() << name;
            }
            else if(tokenName == "desc" )
            {
                desc = xmlReader.readElementText();
                //                qDebug() << desc;
            }
            else if(tokenName == "prngSeed" )
            {
                prngSeed = xmlReader.readElementText();
                //                qDebug() << prngSeed;
            }
            else if(tokenName == "ModelParameters" )
            {
                processModelParameters();
            }
            else if(tokenName == "Dimensions" )
            {
                processDimensions();
            }
            else if(tokenName == "Actor" )
            {
                ProcessActors();

            }
            else if(tokenName == "iaPair" )
            {
                ProcessIdealAdjustments();
            }
            else
            {
            }
        }
        else
        {
        }
    }
    if (xmlReader.hasError())
    {
        qDebug()<<"Error here !!" << xmlReader.errorString();
    }
    else
    {
        organizeXmlData();
    }
    xmlFile->close();
}

void Xmlparser::saveToXmlFile(QStringList parameters, QStandardItemModel *smpData, QStandardItemModel *affMatrix, QString filename)
{
    if(!filename.isEmpty())
    {
        QFile file(filename);
        file.open(QIODevice::WriteOnly);
        QXmlStreamWriter xmlWriter;
        xmlWriter.setAutoFormatting(true);
        xmlWriter.setDevice(&file);

        /* Writes a document start with the XML version number. */
        xmlWriter.writeStartDocument();

        xmlWriter.writeStartElement("Scenario");
        xmlWriter.writeAttribute("xmlns:xsi","http://www.w3.org/2001/XMLSchema-instance");
        xmlWriter.writeAttribute("xsi:noNamespaceSchemaLocation","smpSchema.xsd");

        tagElements(&xmlWriter,"name",parameters.at(0));
        tagElements(&xmlWriter,"desc",parameters.at(1));
        tagElements(&xmlWriter,"prngSeed",parameters.at(2));

        //ModuleParameters
        xmlWriter.writeStartElement("ModelParameters");
        tagElements(&xmlWriter,"VictoryProbModel",parameters.at(3));
        tagElements(&xmlWriter,"PCEModel",parameters.at(4));
        tagElements(&xmlWriter,"StateTransitions",parameters.at(5));
        tagElements(&xmlWriter,"VotingRule",parameters.at(6));
        tagElements(&xmlWriter,"BigRAdjust",parameters.at(7));
        tagElements(&xmlWriter,"BigRRange",parameters.at(8));
        tagElements(&xmlWriter,"ThirdPartyCommit",parameters.at(9));
        tagElements(&xmlWriter,"InterVecBrgn",parameters.at(10));
        tagElements(&xmlWriter,"BargnModel",parameters.at(11));
        //End Module Parameters
        xmlWriter.writeEndElement();

        int dims = parameters.length() - 12;
        //    qDebug()<<dims;

        //Dimensions
        xmlWriter.writeStartElement("Dimensions");
        for(int i = 0 ; i < dims ; ++i)
        {
            tagElements(&xmlWriter,"dName",parameters.at(12+i));
        }
        //end dimensions
        xmlWriter.writeEndElement();

        //Actors
        xmlWriter.writeStartElement("Actors");

        for(int i=0; i< smpData->rowCount();++i)
        {
            xmlWriter.writeStartElement("Actor");
            tagElements(&xmlWriter,"name",smpData->item(i,0)->text());
            tagElements(&xmlWriter,"description",smpData->item(i,1)->text());
            tagElements(&xmlWriter,"capability",smpData->item(i,2)->text());

            xmlWriter.writeStartElement("Position");
            for(int j=0; j< dims; ++j)
            {
                tagElements(&xmlWriter,"dCoord",smpData->item(i,3+(j*2))->text());   //3 5 7 9
            }
            xmlWriter.writeEndElement();//end position

            xmlWriter.writeStartElement("Salience");
            for(int j=0; j< dims; ++j)
            {
                tagElements(&xmlWriter,"dSal",smpData->item(i,4+(j*2))->text()); //4 6 8 10
            }
            xmlWriter.writeEndElement();//end Salience

            xmlWriter.writeEndElement();//end actor
        }
        xmlWriter.writeEndElement();//end Actors

        //Ideal Adjustment
        xmlWriter.writeStartElement("IdealAdjustment");

        for(int i=0; i< affMatrix->rowCount();++i)
        {
            for(int j=0; j<affMatrix->columnCount();++j)
            {
                xmlWriter.writeStartElement("iaPair");
                tagElements(&xmlWriter,"adjustingIdeal",affMatrix->headerData(i,Qt::Horizontal).toString());
                tagElements(&xmlWriter,"referencePos",affMatrix->headerData(j,Qt::Vertical).toString());
                tagElements(&xmlWriter,"adjust",affMatrix->item(i,j)->text());
                xmlWriter.writeEndElement();//end iaPair
            }
        }
        xmlWriter.writeEndElement();//end IdealAdjustment

        /*end scenario */
        xmlWriter.writeEndDocument();

        file.close();
        emit newXmlFilePath(filename);
    }
}

void Xmlparser::saveNewDataToXmlFile(QStringList parameters, QTableWidget *smpDataWidget,
                                     QTableWidget *affintyDataWidget)
{
    QString filename =parameters.at(0);
    if(!filename.endsWith(".xml"))
        filename.append(".xml");

    QFile file(filename);
    file.open(QIODevice::WriteOnly);

    QXmlStreamWriter xmlWriter;
    xmlWriter.setAutoFormatting(true);
    xmlWriter.setDevice(&file);

    /* Writes a document start with the XML version number. */
    xmlWriter.writeStartDocument();

    xmlWriter.writeStartElement("Scenario");
    xmlWriter.writeAttribute("xmlns:xsi","http://www.w3.org/2001/XMLSchema-instance");
    xmlWriter.writeAttribute("xsi:noNamespaceSchemaLocation","smpSchema.xsd");

    tagElements(&xmlWriter,"name",parameters.at(1));
    tagElements(&xmlWriter,"desc",parameters.at(2));
    tagElements(&xmlWriter,"prngSeed",parameters.at(3));

    //ModuleParameters
    xmlWriter.writeStartElement("ModelParameters");
    tagElements(&xmlWriter,"VictoryProbModel",parameters.at(4));
    tagElements(&xmlWriter,"PCEModel",parameters.at(5));
    tagElements(&xmlWriter,"StateTransitions",parameters.at(6));
    tagElements(&xmlWriter,"VotingRule",parameters.at(7));
    tagElements(&xmlWriter,"BigRAdjust",parameters.at(8));
    tagElements(&xmlWriter,"BigRRange",parameters.at(9));
    tagElements(&xmlWriter,"ThirdPartyCommit",parameters.at(10));
    tagElements(&xmlWriter,"InterVecBrgn",parameters.at(11));
    tagElements(&xmlWriter,"BargnModel",parameters.at(12));
    //End Module Parameters
    xmlWriter.writeEndElement();

    int dims = parameters.length() - 13;
    //    qDebug()<<dims;

    //Dimensions
    xmlWriter.writeStartElement("Dimensions");

    for(int i = 0 ; i < dims ; ++i)
    {
        tagElements(&xmlWriter,"dName",parameters.at(13+i));
    }
    //end dimensions
    xmlWriter.writeEndElement();


    //Actors
    xmlWriter.writeStartElement("Actors");

    for(int i=0; i< smpDataWidget->rowCount();++i)
    {
        xmlWriter.writeStartElement("Actor");
        tagElements(&xmlWriter,"name",smpDataWidget->item(i,0)->text());
        tagElements(&xmlWriter,"description",smpDataWidget->item(i,1)->text());
        tagElements(&xmlWriter,"capability",smpDataWidget->item(i,2)->text());

        xmlWriter.writeStartElement("Position");

        for(int j=0; j< dims; ++j)
        {
            tagElements(&xmlWriter,"dCoord",smpDataWidget->item(i,3+(j*2))->text()); // 3 5  7  9
        }
        xmlWriter.writeEndElement();//end position

        xmlWriter.writeStartElement("Salience");
        for(int j=0; j< dims; ++j)
        {
            tagElements(&xmlWriter,"dSal",smpDataWidget->item(i,4+(j*2))->text()); //4 6 8 10
        }
        xmlWriter.writeEndElement();//end Salience

        xmlWriter.writeEndElement();//end actor
    }
    xmlWriter.writeEndElement();//end Actors

    //Ideal Adjustment
    xmlWriter.writeStartElement("IdealAdjustment");

    for(int i=0; i< affintyDataWidget->rowCount();++i)
    {
        for(int j=0; j<affintyDataWidget->columnCount();++j)
        {
            xmlWriter.writeStartElement("iaPair");
            tagElements(&xmlWriter,"adjustingIdeal",affintyDataWidget->horizontalHeaderItem(i)->text());
            tagElements(&xmlWriter,"referencePos",affintyDataWidget->verticalHeaderItem(j)->text());
            tagElements(&xmlWriter,"adjust",affintyDataWidget->item(i,j)->text());
            xmlWriter.writeEndElement();//end iaPair
        }
    }
    xmlWriter.writeEndElement();//end IdealAdjustment

    /*end scenario */
    xmlWriter.writeEndDocument();
    file.close();
    qDebug()<<xmlWriter.hasError();


}

void Xmlparser::updateHomeDir(QString dir)
{
    homeDirectory =dir;
}

void Xmlparser::tagElements(QXmlStreamWriter *xmlWriter, QString tagName, QString tagValue)
{
    xmlWriter->writeStartElement(tagName);
    xmlWriter->writeCharacters(tagValue);
    xmlWriter->writeEndElement();
}

void Xmlparser::processModelParameters()
{
    while (xmlReader.readNext() && !xmlReader.isEndElement())
    {
        QString tokenName = xmlReader.name().toString();
        if (tokenName == "VictoryProbModel")
        {
            // qDebug() << "VictoryProbModel:" << tokenName  << "text:" << xmlReader.readElementText();
            modelParametes.append(xmlReader.readElementText());
        }
        else if(tokenName == "PCEModel" )
        {
            //   qDebug() << "PCEModel:" << tokenName  << "text:" << xmlReader.readElementText();
            modelParametes.append(xmlReader.readElementText());
        }
        else if(tokenName == "StateTransitions" )
        {
            // qDebug() << "StateTransitions:" << tokenName  << "text:" << xmlReader.readElementText();
            modelParametes.append(xmlReader.readElementText());
        }
        else if(tokenName == "VotingRule" )
        {
            //qDebug() << "VotingRule:" << tokenName  << "text:" << xmlReader.readElementText();
            modelParametes.append(xmlReader.readElementText());
        }
        else if(tokenName == "BigRAdjust" )
        {
            //qDebug() << "BigRAdjust:" << tokenName  << "text:" << xmlReader.readElementText();
            modelParametes.append(xmlReader.readElementText());
        }
        else if(tokenName == "BigRRange" )
        {
            //qDebug() << "BigRRange:" << tokenName  << "text:" << xmlReader.readElementText();
            modelParametes.append(xmlReader.readElementText());
        }
        else if(tokenName == "ThirdPartyCommit" )
        {
            //qDebug() << "ThirdPartyCommit:" << tokenName  << "text:" << xmlReader.readElementText();
            modelParametes.append(xmlReader.readElementText());
        }
        else if(tokenName == "InterVecBrgn" )
        {
            //qDebug() << "InterVecBrgn:" << tokenName  << "text:" << xmlReader.readElementText();
            modelParametes.append(xmlReader.readElementText());
        }
        else if(tokenName == "BargnModel" )
        {
            //qDebug() << "BargnModel:" << tokenName  << "text:" << xmlReader.readElementText();
            modelParametes.append(xmlReader.readElementText());
        }

    }
}

void Xmlparser::processDimensions()
{
    while (xmlReader.readNext() && !xmlReader.isEndElement())
    {
        QString tokenName = xmlReader.name().toString();
        if (tokenName == "dName")
        {
            //qDebug() << "dName:" << tokenName  << "text:" << xmlReader.readElementText();
            dimensionNames.append(xmlReader.readElementText());
        }
    }
}

void Xmlparser::ProcessActors()
{
    QStringList actors;
    while (xmlReader.readNext() && !xmlReader.isEndElement() )
    {
        QString tokenName = xmlReader.name().toString();
        if (tokenName == "name")
        {
            //            qDebug() << "name:" << tokenName  << "text:" << xmlReader.readElementText();
            actors.append(xmlReader.readElementText());
        }
        else if(tokenName == "description" )
        {
            //            qDebug() << "description:" << tokenName  << "text:" << xmlReader.readElementText();
            actors.append(xmlReader.readElementText());
        }
        else if(tokenName == "capability" )
        {
            //            qDebug() << "capability:" << tokenName  << "text:" << xmlReader.readElementText();
            actors.append(xmlReader.readElementText());
        }

        else if(tokenName == "dCoord" )
        {
            //            qDebug() << "dCoord:" << tokenName  << "text:" << xmlReader.readElementText();
            actors.append(xmlReader.readElementText());
        }

    }
    while (xmlReader.readNext() && !xmlReader.isEndElement() )
    {
        QString tokenName = xmlReader.name().toString();
        if(tokenName == "dSal" )
        {
            //            qDebug() << "dSal:" << tokenName  << "text:" << xmlReader.readElementText();
            actors.append(xmlReader.readElementText());
        }
    }
    actorsList.append(actors);
    actors.clear();
}

void Xmlparser::ProcessIdealAdjustments()
{
    QStringList idealAdj;
    while (xmlReader.readNext() && !xmlReader.isEndElement())
    {
        QString tokenName = xmlReader.name().toString();
        if (tokenName == "adjustingIdeal")
        {
            //            qDebug() << "adjustingIdeal:" << tokenName  << "text:" << xmlReader.readElementText();
            idealAdj.append(xmlReader.readElementText());
        }
        else if(tokenName == "referencePos" )
        {
            //            qDebug() << "referencePos:" << tokenName  << "text:" << xmlReader.readElementText();
            idealAdj.append(xmlReader.readElementText());
        }
        else if(tokenName == "adjust" )
        {
            //            qDebug() << "adjust:" << tokenName  << "text:" << xmlReader.readElementText();
            idealAdj.append(xmlReader.readElementText());
        }
    }
    idealAdjustmentList.append(idealAdj);
    idealAdj.clear();
}

void Xmlparser::organizeXmlData()
{
    QStringList modelDesc;
    modelDesc.append(name);
    modelDesc.append(desc);
    modelDesc.append(prngSeed);

    //smp actors data
    for(int row = 0; row < actorsList.length() ; ++row)
    {
        QString precision;
        for(int col = 0; col < 3 ; ++ col)
        {
            xmlModel->setItem(row,col, new QStandardItem(actorsList.at(row).at(col)));
        }

        int col =2;
        int dim = (actorsList.at(0).length()-3)/2;
        int baseA =2;
        int baseS = baseA + dim;

        for(int index =1; index <= dim; ++index)
        {
            int ndxDataA = baseA + index;
            int ndxDataB = baseS + index;

            precision = actorsList.at(row).at(ndxDataA);
            precision = QString::number(precision.toDouble(),'f',2);
            xmlModel->setItem(row,++col, new QStandardItem(precision));

            precision = actorsList.at(row).at(ndxDataB);
            precision = QString::number(precision.toDouble(),'f',2);
            xmlModel->setItem(row,++col, new QStandardItem(precision));
        }
    }
    emit xmlParsedData(modelDesc,modelParametes,dimensionNames,xmlModel,idealAdjustmentList);
}
