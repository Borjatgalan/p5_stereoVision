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
    initVecinos();

    //Inicializacion de imagenes
    colorImage.create(240, 320, CV_8UC3);
    grayImage.create(240, 320, CV_8UC1);
    destColorImage.create(240, 320, CV_8UC3);
    destGrayImage.create(240, 320, CV_8UC1);
    destGrayImage2.create(240, 320, CV_8UC1);
    groundTruthImage.create(240,320,CV_8UC1);
    canny_image.create(240, 320, CV_8UC1);
    detected_edges.create(240, 320, CV_8UC1);
    imgRegiones.create(240,320, CV_32SC1);
    imgMask.create(240, 320, CV_8UC1);
    corners.create(240, 320, CV_8UC1);
    cornersD.create(240, 320, CV_8UC1);
    fijos.create(240, 320, CV_8UC1);
    disparidad.create(240, 320, CV_32FC1);

    visorS = new ImgViewer(&grayImage, ui->imageFrameS);
    visorD = new ImgViewer(&destGrayImage, ui->imageFrameD);
    visorS2 = new ImgViewer(&destGrayImage2, ui->visorDisp);
    visorGroundTruth = new ImgViewer(&groundTruthImage, ui->visorTDisp);

    connect(&timer, SIGNAL(timeout()), this, SLOT(compute()));

    connect(ui->loadButton, SIGNAL(pressed()), this, SLOT(loadFromFile()));
    connect(ui->loadButton_2, SIGNAL(pressed()),this,SLOT(loadGroundTruth()));
    connect(ui->loadButton_3, SIGNAL(pressed()),this,SLOT(loadFromFile2()));
    connect(ui->initButton, SIGNAL(pressed()),this,SLOT(initProcess()));
    connect(ui->corners_checkbox, SIGNAL(pressed()),this,SLOT(printCorners()));

    connect(visorS2, SIGNAL(windowSelected(QPointF,int,int)), this, SLOT(printLCDdisparity(QPointF)));

    timer.start(30);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete visorS;
    delete visorD;
    delete visorS2;
    delete visorGroundTruth;
}

void MainWindow::compute()
{
    if(ui->corners_checkbox->isChecked()){
        printCorners();
    }



    visorS->update();
    visorD->update();
    visorS2->update();
    visorGroundTruth->update();
}

/** Ejecucion de todas las fases de la vision estereo
 * @brief MainWindow::initProcess
 */
void MainWindow::initProcess()
{
    segmentation();
    cornerDetection();
    initDisparity();
    initDisparity2();
    allocate_non_fixed_points();
    printDisparity();

}

/** Generamos las imagenes de esquinas de la ventana izquierda y de la ventana derecha
 * @brief MainWindow::cornerDetection
 */
void MainWindow::cornerDetection()
{
    destGrayImage2.setTo(0);

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
    //Inicializamos el mapa de esquinas de la ventana izquierda
    for (size_t i = 0 ; i < cornerList.size(); i++) {
        corners.at<uchar>(cornerList[i].point.y, cornerList[i].point.x) = 1;
    }

    for (size_t k = 0 ; k < cornerListD.size();  k++) {
        cornersD.at<uchar>(cornerListD[k].point.y, cornerListD[k].point.x) = 1;
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

        image = cv::imread(fileName.toStdString());
        cv::resize(image, image, Size(320, 240));
        cvtColor(image, grayImage, COLOR_RGB2GRAY);
        width = image.cols;
        connect(&timer, SIGNAL(timeout()), this, SLOT(compute()));
    }
}

void MainWindow::loadFromFile2()
{
    disconnect(&timer, SIGNAL(timeout()), this, SLOT(compute()));

    Mat image;
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open"), "/home", tr("Images (*.jpg *.png "
                                                                                  "*.jpeg *.gif);;All Files(*)"));

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

        image = cv::imread(fileName.toStdString());
        cv::resize(image, image, Size(320, 240));
        cvtColor(image, destGrayImage, COLOR_RGB2GRAY);
        width = image.cols;
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
    int lowThreshold = 40;
    int const maxThreshold = 120;

    grayImage.copyTo(detected_edges);

    // Canny detector
    cv::Canny(detected_edges, canny_image, lowThreshold, maxThreshold, 3);
    canny_image.copyTo(detected_edges);

    grayImage.copyTo(destGrayImage2, canny_image);

    //Initialize regions img  and region list

    imgRegiones.setTo(-1);
    listRegiones.clear();
    //initialize mask image
    cv::copyMakeBorder(canny_image,imgMask,1,1,1,1,1, BORDER_DEFAULT);

}

/** Procesamiento de la imagen
 * @brief MainWindow::segmentation
 */
void MainWindow::segmentation(){

    initialize();

    idReg = 0;
    Point seedPoint;
    int grisAcum;
    int flags = 4|(1 << 8)| FLOODFILL_MASK_ONLY | FLOODFILL_FIXED_RANGE;

    for(int i = 0; i < imgRegiones.rows; i++){
        for(int j = 0; j < imgRegiones.cols; j++){
            if(imgRegiones.at<int>(i,j) == -1 && detected_edges.at<uchar>(i,j) != 255){
                seedPoint.x = j;
                seedPoint.y = i;
                //Comprobación de rango flotante
                cv::floodFill(grayImage, imgMask, seedPoint,idReg, &minRect,Scalar(30), Scalar(30), flags);

                grisAcum = 0;
                r.nPuntos = 0;
                for(int k = minRect.x; k < minRect.x+minRect.width; k++){   //columnas
                    for(int z = minRect.y; z < minRect.y+minRect.height; z++){  //filas
                        if(imgMask.at<uchar>(z+1, k+1) == 1 && imgRegiones.at<int>(z, k) == -1){
                            r.id = idReg;
                            r.nPuntos++;
                            r.pIni = Point(k,z);                                //Point(columna, fila)
                            grisAcum += grayImage.at<uchar>(z, k);
                        }
                        imgRegiones.at<int>(z, k) = idReg;
                    }
                }
                r.gMedio = grisAcum / r.nPuntos;
                listRegiones.push_back(r);
                idReg++;
            }
        }
    }


    // ######### POST-PROCESAMIENTO #########

    asignarBordesARegion();
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



/** Primera fase de inicializacion
 * @brief MainWindow::initDisparity
 */
void MainWindow::initDisparity()
{
    Mat result; //resultado en (0,0) de tipo float
    float mejorR; //mejor resultado
    result.create(1, 1, CV_32F);
    float umbral = 0.8;
    int xD = 0;
    int mejorxD = 0;

    for(size_t it = 0; it < cornerList.size(); it++){
        int xI = cornerList[it].point.x;
        int yI = cornerList[it].point.y;
        result.setTo(0);    //Resultado de similitud
        mejorR = 0;         //Mejor valor de similtud hasta ahora
        for(xD = 0; xD < cornersD.cols; xD++){
            if((xI-W/2) >= 0 && (xI+W/2) < 320 && (xD-W/2) >= 0 && (xD+W/2) < 320 && (yI-W/2) >= 0 && (yI+W/2)<240){
                if(cornersD.at<uchar>(yI, xD) == 1){
                    Mat winI = destGrayImage(cv::Rect(xI-W/2, yI-W/2, W, W));
                    Mat winD = grayImage(cv::Rect(xD-W/2, yI-W/2, W, W));
                    matchTemplate(winI,winD,result,TM_CCOEFF_NORMED);
                    if(result.at<float>(0) >= mejorR){
                        mejorR = result.at<float>(0);
                        mejorxD = xD;
                    }
                }
            }
        }
        //Comprobamos si cumple un valor de correspondencia aceptable
        if(mejorR >= umbral){
            fijos.at<uchar>(yI, xI) = 1;
            disparidad.at<float>(yI, xI) = xI - mejorxD;
        }
    }
}

/** Segunda fase: Calculo de la media de disparidad de cada region
 * @brief MainWindow::initDisparity2
 */
void MainWindow::initDisparity2()
{
    int id = 0;
    for(size_t i = 0; i < listRegiones.size(); i++){
        listRegiones[i].dMedia = 0;
        listRegiones[i].nPuntosFijos = 0;
    }

    for(int i = 0; i < imgRegiones.rows; i++){
        for(int j = 0; j < imgRegiones.cols; j++){
            if(fijos.at<uchar>(i,j) == 1){
                id = imgRegiones.at<int>(i,j);
                listRegiones[id].dMedia += disparidad.at<float>(i,j);
                listRegiones[id].nPuntosFijos++;
            }
        }
    }

    for(int i = 0; i < imgRegiones.rows; i++){
        if(listRegiones[i].nPuntosFijos > 0){
            listRegiones[i].dMedia = listRegiones[i].dMedia / listRegiones[i].nPuntosFijos;
        }
        else{
            listRegiones[i].dMedia = 0;
        }
    }
}

/** Representacion visual de las esquinas
 * @brief MainWindow::printCorners
 */
void MainWindow::printCorners()
{
    initDisparity();
    if(!cornerList.empty()){
        for(size_t i = 0; i < cornerList.size();i++){
            if(fijos.at<uchar>(cornerList[i].point.y, cornerList[i].point.x) == 0){
                visorS->drawSquare(QPointF(cornerList[i].point.x,cornerList[i].point.y), 3,3, Qt::red);
            }
            else{
                visorS->drawSquare(QPointF(cornerList[i].point.x,cornerList[i].point.y), 3,3, Qt::green);
            }
        }
    }

    if(!cornerListD.empty()){
        for(size_t i = 0; i < cornerListD.size(); i++){
            if(fijos.at<uchar>(cornerListD[i].point.y, cornerListD[i].point.x) == 0){
                visorD->drawSquare(QPointF(cornerListD[i].point.x, cornerListD[i].point.y), 3,3, Qt::red);
            }
            else{
                visorD->drawSquare(QPointF(cornerListD[i].point.x, cornerListD[i].point.y), 3,3, Qt::green);
            }
        }
    }

}

void MainWindow::printDisparity()
{
    int gris = 0;
    for(int i = 0; i < destGrayImage2.rows; i++){
        for(int j = 0; j < destGrayImage2.cols; j++){
            gris = disparidad.at<float>(i,j) * 3. * width/320.; //Resolucion original de las camaras
            destGrayImage2.at<uchar>(i,j) = (uchar) gris;
        }
    }
}

/** Proceso de asignacion de un valor de disparidad a puntos no fijos
 * @brief MainWindow::allocate_non_fixed_points
 */
void MainWindow::allocate_non_fixed_points()
{
    int id = 0;
    float disp = 0;
    for(int i = 0; i < disparidad.rows; i++){
        for(int j = 0; j < disparidad.cols; j++){
            if(fijos.at<uchar>(i,j) == 0){
                id = imgRegiones.at<int>(i, j);
                disp = listRegiones[id].dMedia;
                disparidad.at<float>(i, j) = disp;
            }
        }
    }
}


/** Despliega por pantalla LCD el valor estimado y real de la disparidad.
 * @brief MainWindow::printLCDdisparity
 * @param punto
 */
void MainWindow::printLCDdisparity(QPointF punto)
{
    ui->estimated_lcd->display(destGrayImage2.at<uchar>(punto.y(), punto.x()));
    ui->true_lcd->display(groundTruthImage.at<uchar>(punto.y(), punto.x()));
}





