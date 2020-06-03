#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/videoio/videoio.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include <imgviewer.h>

#include <QtWidgets/QFileDialog>

#define W 11

/**
 * P5 - Stereo Vision
 * Ivan González Domínguez
 * Borja Alberto Tirado Galán
 *
 *
 */

using namespace cv;

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    typedef struct{
        Point point;
        float valor;
    } punto;

    struct puntoCompare {
        bool operator()(const punto a, const punto b) const {
            return a.valor > b.valor;
        }
    };

    typedef struct{
        int id;
        Point pIni;
        int nPuntos;
        int nPuntosFijos;
        uchar gMedio;   //valor gris medio
        Vec3b rgbMedio; //valor color medio
        float dMedia; //valor de disparidad media
    }Region;

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    //Interfaz principal de usuario
    Ui::MainWindow *ui;

    QTimer timer;

    ImgViewer *visorS, *visorD, *visorS2, *visorGroundTruth;
    Mat colorImage, grayImage, destColorImage, destGrayImage, destGrayImage2, groundTruthImage;
    bool winSelected, selectColorImage;
    Rect imageWindow;
    int idReg;
    int width;          //Anchura de la imagen original

    Mat corners;        //Mat de esquinas izquierda
    Mat cornersD;       //Mat de esquinas derecha
    Mat imgRegiones;    //Imagen de regiones
    Mat imgMask;        //Imagen de mascara
    Mat detected_edges; //Mat de bordes
    Mat canny_image;    //Mat de canny
    Mat fijos;          //Mat de puntos fijos de la imagen izquierda
    Mat disparidad;     //Mapa de disparidad
    Rect minRect;       //Minima ventana de los puntos modificados (añadidos a la region)
    Region r;           //Estructura de tipo Region


    std::vector<Region> listRegiones;   //Lista de regiones
    std::vector<Point> vecinos;         //Lista de vecinos
    std::vector<punto> cornerList;      //Lista de esquinas ventana izquierda
    std::vector<punto> cornerListD;     //Lista de esquinas ventana derecha
    std::vector<QLine> lineList;        //Lista de lineas
    std::vector<Point> pCorte;          //Lista de puntos validos
    std::vector<QLine> segmentList;     //Lista de segmentos

public slots:
    void compute();
    void selectWindow(QPointF p, int w, int h);
    void deselectWindow();
    void loadFromFile();
    void loadFromFile2();
    void loadGroundTruth();
    void saveToFile();
    void initialize();
    void segmentation();
    void initVecinos();
    int vecinoMasSimilar(int x, int y);
    void asignarBordesARegion();
    void mostrarListaRegiones();

    //Metodos practica 5
    void initProcess();
    void cornerDetection();
    void initDisparity();
    void initDisparity2();
    void allocate_non_fixed_points();
    void printCorners();
    void printDisparity();
    void printLCDdisparity(QPointF punto);
};


#endif // MAINWINDOW_H
