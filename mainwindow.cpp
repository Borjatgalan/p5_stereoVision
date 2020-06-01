#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QMessageBox"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

/**
 * P5 - Stereo Vision
 * Ivan González Domínguez
 * Borja Alberto Tirado Galán
 *
 *
 */

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    winSelected = false;
    selectColorImage = false;
    initVecinos();

    //Inicializacion de imagenes
    colorImage.create(240, 320, CV_8UC3);
    grayImage.create(240, 320, CV_8UC1);
    destColorImage.create(240, 320, CV_8UC3);
    destGrayImage.create(240, 320, CV_8UC1);
    groundTruthImage.create(240,320,CV_8UC1);
    canny_image.create(240, 320, CV_8UC1);
    detected_edges.create(240, 320, CV_8UC1);
    imgRegiones.create(240,320, CV_32SC1);
    imgMask.create(240, 320, CV_8UC1);
    corners.create(240, 320, CV_8UC1);
    fijos.create(240, 320, CV_8UC1);
    disparidad.create(240, 320, CV_32FC1);

    visorS = new ImgViewer(&grayImage, ui->imageFrameS);
    visorD = new ImgViewer(&destGrayImage, ui->imageFrameD);
    visorGroundTruth = new ImgViewer(&groundTruthImage, ui->visorTDisp);

    connect(&timer, SIGNAL(timeout()), this, SLOT(compute()));
    connect(ui->colorButton, SIGNAL(clicked(bool)), this, SLOT(change_color_gray(bool)));
    connect(visorS, SIGNAL(windowSelected(QPointF, int, int)), this, SLOT(selectWindow(QPointF, int, int)));
    connect(visorS, SIGNAL(pressEvent()), this, SLOT(deselectWindow()));

    connect(ui->loadButton, SIGNAL(pressed()), this, SLOT(loadFromFile()));
    connect(ui->loadButton_2, SIGNAL(pressed()),this,SLOT(loadGroundTruth()));




    timer.start(30);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete visorS;
    delete visorD;
}

void MainWindow::compute()
{



    if (winSelected)
    {
        visorS->drawSquare(QPointF(imageWindow.x + imageWindow.width / 2, imageWindow.y + imageWindow.height / 2), imageWindow.width, imageWindow.height, Qt::green);
    }
    visorS->update();
    visorD->update();
    visorGroundTruth->update();
}



void MainWindow::change_color_gray(bool color)
{
    if (color)
    {
        ui->colorButton->setText("Gray image");
        visorS->setImage(&colorImage);
        visorD->setImage(&destColorImage);
    }
    else
    {
        ui->colorButton->setText("Color image");
        visorS->setImage(&grayImage);
        visorD->setImage(&destGrayImage);
    }
}

void MainWindow::selectWindow(QPointF p, int w, int h)
{
    QPointF pEnd;
    if (w > 0 && h > 0)
    {
        imageWindow.x = p.x() - w / 2;
        if (imageWindow.x < 0)
            imageWindow.x = 0;
        imageWindow.y = p.y() - h / 2;
        if (imageWindow.y < 0)
            imageWindow.y = 0;
        pEnd.setX(p.x() + w / 2);
        if (pEnd.x() >= 320)
            pEnd.setX(319);
        pEnd.setY(p.y() + h / 2);
        if (pEnd.y() >= 240)
            pEnd.setY(239);
        imageWindow.width = pEnd.x() - imageWindow.x;
        imageWindow.height = pEnd.y() - imageWindow.y;

        winSelected = true;
    }
}

void MainWindow::deselectWindow()
{
    winSelected = false;
}

void MainWindow::loadFromFile()
{
    disconnect(&timer, SIGNAL(timeout()), this, SLOT(compute()));

    Mat image;
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open"), "/home", tr("Images (*.jpg *.png "
                                                                                  "*.jpeg *.gif);;All Files(*)"));
    image = cv::imread(fileName.toStdString());

    if (fileName.isEmpty())
        return;
    else
    {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly))
        {
            QMessageBox::information(this, tr("Unable to open file"), file.errorString());
            return;
        }
        ui->captureButton->setChecked(false);
        ui->captureButton->setText("Start capture");
        cv::resize(image, colorImage, Size(320, 240));
        cvtColor(colorImage, colorImage, COLOR_BGR2RGB);
        cvtColor(colorImage, grayImage, COLOR_RGB2GRAY);

        if (ui->colorButton->isChecked())
            colorImage.copyTo(destColorImage);
        else
            grayImage.copyTo(destGrayImage);
        connect(&timer, SIGNAL(timeout()), this, SLOT(compute()));
    }
}

void MainWindow::loadGroundTruth()
{
    disconnect(&timer, SIGNAL(timeout()), this, SLOT(compute()));

    Mat image;
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open"), "/home", tr("Images (*.jpg *.png "
                                                                                  "*.jpeg *.gif);;All Files(*)"));
    image = cv::imread(fileName.toStdString());

    if (fileName.isEmpty())
        return;
    else
    {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly))
        {
            QMessageBox::information(this, tr("Unable to open file"), file.errorString());
            return;
        }
        ui->loadButton_2->setChecked(false);
        ui->captureButton->setText("Start capture");
        cv::resize(image, groundTruthImage, Size(320, 240));
        cvtColor(groundTruthImage, groundTruthImage, COLOR_BGR2RGB);
        cvtColor(groundTruthImage, groundTruthImage, COLOR_RGB2GRAY);

        connect(&timer, SIGNAL(timeout()), this, SLOT(compute()));
    }
}

void MainWindow::saveToFile()
{
    disconnect(&timer, SIGNAL(timeout()), this, SLOT(compute()));
    Mat save_image;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image File"),
                                                    QString(),
                                                    tr("JPG (*.JPG) ; jpg (*.jpg); png (*.png); jpeg(*.jpeg); gif(*.gif); All Files (*)"));
    if (ui->colorButton->isChecked())
        cvtColor(destColorImage, save_image, COLOR_RGB2BGR);

    else
        cvtColor(destGrayImage, save_image, COLOR_GRAY2BGR);

    if (fileName.isEmpty())
        return;
    else
    {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly))
        {
            QMessageBox::information(this, tr("Unable to open file"),
                                     file.errorString());
            return;
        }
    }
    cv::imwrite(fileName.toStdString(), save_image);

    connect(&timer, SIGNAL(timeout()), this, SLOT(compute()));
}

void MainWindow::initialize(){
    //INICIALIZA PARÁMETROS COMO IMAGEN DE MÁSCARA Y HACE EL GUARDADO DE LA IMAGEN CANNY
    int lowThreshold = 40;
    int const maxThreshold = 120;
    if(ui->colorButton->isChecked()){
        // Reduce noise with a kernel 3x3
        blur(colorImage, detected_edges, Size(3, 3));

        // Canny detector
        cv::Canny(detected_edges, canny_image, lowThreshold, maxThreshold, 3);
        canny_image.copyTo(detected_edges);

        grayImage.copyTo(destColorImage, canny_image);
    }
    else{
        // Reduce noise with a kernel 3x3
        blur(grayImage, detected_edges, Size(3, 3));

        // Canny detector
        cv::Canny(detected_edges, canny_image, lowThreshold, maxThreshold, 3);
        canny_image.copyTo(detected_edges);

        grayImage.copyTo(destGrayImage, canny_image);
    }

    //Initialize regions img  and region list
    imgRegiones.setTo(-1);
    listRegiones.clear();
    //initialize mask image
    cv::copyMakeBorder(canny_image,imgMask,1,1,1,1,1, BORDER_DEFAULT);

}

/** SE ENCARGA DEL PROCESAMIENTO DE LA IMAGEN
 * @brief MainWindow::segmentation
 */
void MainWindow::segmentation(){
    /*
    initialize();
    idReg = 0;
    Point seedPoint;
    int grisAcum, R_Acum, G_Acum, B_Acum;

    for(int i = 0; i<imgRegiones.rows; i++){
        for(int j = 0; j<imgRegiones.cols; j++){
            if(imgRegiones.at<int>(i,j) == -1 && detected_edges.at<uchar>(i,j) != 255){
                seedPoint.x = j;
                seedPoint.y = i;
                //Comprobación de imagen en color o grises
                if(ui->colorButton->isChecked()){
                    //Comprobación de punto flotante o fijo
                    if(ui->showFloatingRange_checkbox->isChecked()){
                        cv::floodFill(colorImage, imgMask, seedPoint,idReg, &minRect,
                                      Scalar(ui->max_box->value(), ui->max_box->value(), ui->max_box->value()),
                                      Scalar(ui->max_box->value(), ui->max_box->value(), ui->max_box->value()),
                                      4|(1 << 8)| FLOODFILL_MASK_ONLY);
                    }else{
                        cv::floodFill(colorImage, imgMask, seedPoint,idReg, &minRect,
                                      Scalar(ui->max_box->value(), ui->max_box->value(), ui->max_box->value()),
                                      Scalar(ui->max_box->value(), ui->max_box->value(), ui->max_box->value()),
                                      4|(1 << 8)| FLOODFILL_MASK_ONLY | FLOODFILL_FIXED_RANGE);
                    }
                }else{
                    //Comprobación de punto fijo o flotante
                    if(ui->showFloatingRange_checkbox->isChecked()){
                        cv::floodFill(grayImage, imgMask, seedPoint,idReg, &minRect,Scalar(ui->max_box->value()),Scalar(ui->max_box->value()),4|(1 << 8)| FLOODFILL_MASK_ONLY);
                    }else{
                        cv::floodFill(grayImage, imgMask, seedPoint,idReg, &minRect,Scalar(ui->max_box->value()),Scalar(ui->max_box->value()),4|(1 << 8)| FLOODFILL_MASK_ONLY | FLOODFILL_FIXED_RANGE);
                    }
                }

                grisAcum = 0;
                R_Acum = 0;
                G_Acum = 0;
                B_Acum = 0;
                r.nPuntos = 0;
                for(int k = minRect.x; k < minRect.x+minRect.width; k++){ 		//columnas
                    for(int z = minRect.y; z < minRect.y+minRect.height; z++){ 	//filas
                        if(imgMask.at<uchar>(z+1, k+1) == 1 && imgRegiones.at<int>(z, k) == -1){
                            r.id = idReg;
                            r.nPuntos++;
                            r.pIni = Point(k,z);                                //Point(columna, fila)
                            if(ui->colorButton->isChecked()){
                                Vec3b rgb = colorImage.at<Vec3b>(z, k);
                                R_Acum += rgb[0];
                                G_Acum += rgb[1];
                                B_Acum += rgb[2];
                            }
                            else{
                                grisAcum += grayImage.at<uchar>(z, k);
                            }
                            imgRegiones.at<int>(z, k) = idReg;
                        }
                    }
                }
                if(ui->colorButton->isChecked()){
                    r.rgbMedio[0] = R_Acum/r.nPuntos;
                    r.rgbMedio[1] = G_Acum/r.nPuntos;
                    r.rgbMedio[2] = B_Acum/r.nPuntos;
                }
                else{
                    r.gMedio = grisAcum / r.nPuntos;
                }
                listRegiones.push_back(r);
                idReg++;
            }
        }
    }

    // ######### POST-PROCESAMIENTO #########

    asignarBordesARegion();
    vecinosFrontera();
    bottomUp();
*/
}
/** Metodo que agrega a la lista los puntos frontera de la imagen
 * @brief MainWindow::vecinosFrontera
 */
void MainWindow::vecinosFrontera()
{
    int vx = 0, vy = 0, id = 0;
    for(int x = 0; x < imgRegiones.rows; x++){
        for(int y = 0; y <imgRegiones.cols; y++){
            for(size_t i = 0; i < vecinos.size(); i++){
                vx = vecinos[i].x;
                vy = vecinos[i].y;
                if(((x + vx) < imgRegiones.rows) && ((y + vy) < imgRegiones.cols)){
                    if(imgRegiones.at<int>(x, y) != imgRegiones.at<int>(x+vx, y+vy)){
                        id=imgRegiones.at<int>(x, y);
                        listRegiones[id].frontera.push_back(Point(y, x));
                        break;
                    }
                }
            }
        }
    }
}

/** Metodo que visita los 8 vecinos para elegir el más similar al punto central y devuelve el identificador de region.
 * @brief MainWindow::vecinoMasSimilar
 * @param x
 * @param y
 * @return
 */
int MainWindow::vecinoMasSimilar(int x, int y)
{
    int vx = 0, vy = 0;
    int masSimilar = 255;
    int resta;
    int idReg = -1;
    for(size_t i = 0; i < vecinos.size(); i++){
        vx = vecinos[i].x;
        vy = vecinos[i].y;
        //Comprobamos dentro del rango de la imagen
        if((x + vx) >= 0 && (y + vy) >= 0 && (x + vx) < imgRegiones.rows && (y + vy) < imgRegiones.cols){
            if(imgRegiones.at<int>(x+vx, y+vy) != -1){
                if(ui->colorButton->isChecked()){
                    resta = abs(colorImage.at<uchar>(x, y) - colorImage.at<uchar>(x+vx, y+vy));
                }
                else{
                    resta = abs(grayImage.at<uchar>(x, y) - grayImage.at<uchar>(x+vx, y+vy));
                }
                if(resta == 0){
                    return idReg = imgRegiones.at<int>(x+vx, y+vy);

                }else if(resta < masSimilar){
                    masSimilar = resta;
                    idReg = imgRegiones.at<int>(x+vx, y+vy);
                }
            }
        }
    }
    return idReg;
}

/** Inicializa la estructura de visitado de vecinos, el punto (0,0) no se inserta
 * @brief MainWindow::initVecinos
 */
void MainWindow::initVecinos()
{
    /*
     * a  |  b  |   c
     * d  |  p  |   e
     * f  |  g  |   h
     */

    //  Point(columna, fila)
    vecinos.push_back(Point(-1,-1)); //a   //Etiquetas corregidas
    vecinos.push_back(Point( 0,-1)); //b
    vecinos.push_back(Point(+1,-1)); //c
    vecinos.push_back(Point(-1, 0)); //d
    vecinos.push_back(Point(+1, 0)); //e
    vecinos.push_back(Point(-1,+1)); //f
    vecinos.push_back(Point( 0,+1)); //g
    vecinos.push_back(Point(+1,+1)); //h


}

/** Asigna en destGrayImage los valores de gris medio que se encuentran en la imagen y en la lista de regiones
 * @brief MainWindow::bottomUp
 */
void MainWindow::bottomUp()
{
    int id = 0;
    uchar valor = 0;
    Vec3b colorValue;
    Mat imgGris;
    Mat imgColor;
    imgGris.create(240, 320, CV_8UC1);
    imgColor.create(240,320, CV_8UC3);
    if(ui->colorButton->isChecked()){
        for(int y = 0; y < imgRegiones.rows; y++){
            for(int x = 0; x <imgRegiones.cols; x++){
                id = imgRegiones.at<int>(y,x);
                if(id == -1){
                    imgColor.at<uchar>(y,x) = 0;
                }else{
                    colorValue = listRegiones[id].rgbMedio;
                    imgColor.at<Vec3b>(y,x) = colorValue;
                }
            }
        }
        imgColor.copyTo(destColorImage);
    }else{
        for(int y = 0; y < imgRegiones.rows; y++){
            for(int x = 0; x <imgRegiones.cols; x++){
                id = imgRegiones.at<int>(y,x);
                if(id == -1){
                    imgGris.at<uchar>(y,x) = 0;
                }else{
                    valor = listRegiones[id].gMedio;
                    imgGris.at<uchar>(y,x) = valor;
                }
            }
        }
        imgGris.copyTo(destGrayImage);
    }
}

/** Metodo encargado de asignar los bordes a una de las posibles regiones de la imagen
 * @brief MainWindow::asignarBordesARegion
 */
void MainWindow::asignarBordesARegion()
{
    int idVecino;
    for(int i = 0; i<imgRegiones.rows; i++){
        for(int j = 0; j<imgRegiones.cols; j++){
            if(imgRegiones.at<int>(i,j) == - 1){
                idVecino = vecinoMasSimilar(i, j);
                imgRegiones.at<int>(i,j) = idVecino;
                listRegiones[idVecino].nPuntos++;

            }
        }
    }
}

/**
 * Asignar puntos de bordes a alguna region
 * Le asignamos el idReg del vecino que mas se parezca
 */
void MainWindow::mostrarListaRegiones()
{
    Region r;
    qDebug()<<"Size lista regiones: "<<listRegiones.size();
    for(size_t i = 0; i < listRegiones.size(); i++){
        qDebug()<<"ID: "<< listRegiones[i].id;
        qDebug()<<"Punto ini: columna:"<< listRegiones[i].pIni.x << "fila: " << listRegiones[i].pIni.y;
        qDebug()<<"Gris medio: "<< listRegiones[i].gMedio;
        qDebug()<<"Numero de puntos de la region: "<< listRegiones[i].nPuntos;

    }
}

// Generamos las listas de esquinas de winI y winD
void MainWindow::cornerDetection()
{
    Mat dst, dstD;
    dst.create(240, 320, CV_32FC1);
    dstD.create(240, 320, CV_32FC1);

    cornerList.clear();
    cornerListD.clear();
    float threshold = 0.00001;
    dst = Mat::zeros(grayImage.size(), CV_32FC1);
    dstD =Mat::zeros(destGrayImage.size(), CV_32FC1);


    //Metodo que calcula las esquinas
    cv::cornerHarris(grayImage, dst, 5, 3, 0.08);
    cv::cornerHarris(destGrayImage, dstD, 5, 3, 0.08);

    //Almacenamiento de las esquinas en una lista
    for (int x = 0; x < dst.rows; x++){
        for (int y = 0; y < dst.cols; y++){
            if (dst.at<float>(x, y) > threshold)
            {
                punto p;
                p.point = Point(y, x);
                p.valor = dst.at<float>(x, y);
                cornerList.push_back(p);
            }

            if (dstD.at<float>(x, y) > threshold)
            {
                punto p;
                p.point = Point(y, x);
                p.valor = dst.at<float>(x, y);
                cornerListD.push_back(p);
            }
        }
    }
    //Lista ordenada de esquinas
    std::sort(cornerList.begin(), cornerList.end(), puntoCompare());
    std::sort(cornerListD.begin(), cornerListD.end(), puntoCompare());

    //Supresion del no maximo en cornerList
    for (int i = 0; i < (int)cornerList.size(); i++){
        for (int j = i + 1; j < (int)cornerList.size(); j++){
            if (abs(cornerList[i].point.x - cornerList[j].point.x) < 5 &&
                    abs(cornerList[i].point.y - cornerList[j].point.y) < 5)
            {

                cornerList.erase(cornerList.begin() + j);
                j--;
            }
        }
    }
    //Supresion del no maximo en cornerListD
    for (int i = 0; i < (int)cornerListD.size(); i++){
        for (int j = i + 1; j < (int)cornerListD.size(); j++){
            if (abs(cornerListD[i].point.x - cornerListD[j].point.x) < 5 &&
                    abs(cornerListD[i].point.y - cornerListD[j].point.y) < 5)
            {

                cornerListD.erase(cornerListD.begin() + j);
                j--;
            }
        }
    }

    for (size_t i = 0 ;i < cornerList.size();i++) {
        corners.at<uchar>(cornerList[i].point.y, cornerList[i].point.x) = 1;
    }

    for (size_t i = 0 ;i < cornerListD.size();i++) {
        cornersD.at<uchar>(cornerListD[i].point.y, cornerListD[i].point.x) = 1;
    }


}

void MainWindow::initDisparity()
{
    Mat result; //resultado en (0,0) de tipo float
    Mat mejorR; //mejor resultado
    result.create(1, 1, CV_32F);
    mejorR.create(1, 1, CV_32F);
    float umbral = 0.8;
    int xD = 0;

    for(size_t it = 0; it < cornerList.size(); it++){
        int xI = cornerList[it].point.x;
        int yI = cornerList[it].point.y;
        result.setTo(0);    //Resultado de similitud
        mejorR.setTo(0);    //Mejor valor de similtud hasta ahora
        for(xD = 0; xD < cornersD.cols; xD++){
            if(cornersD.at<uchar>(yI, xD) == 1){
                Mat winI = destGrayImage(cv::Rect(xI-W/2, yI-W/2, W, W));
                Mat winD = grayImage(cv::Rect(xD-W/2, yI-W/2, W, W));
                matchTemplate(winI,winD,result,TM_CCOEFF_NORMED);
                if(result.at<float>(0) >= mejorR.at<float>(0)){
                    mejorR.at<float>(0) = result.at<float>(0);
                }
            }
        }
        //Comprobamos si cumple un valor de correspondencia aceptable
        if(mejorR.at<float>(0) >= umbral){
            fijos.at<uchar>(yI, xI) = 1;
            disparidad.at<float>(yI, xI) = xI - xD;
        }
    }

}





