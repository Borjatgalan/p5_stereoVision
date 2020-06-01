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
        std::vector<Point> pFijos;
        std::vector<Point> frontera;
    }Region;

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    //Interfaz principal de usuario
    Ui::MainWindow *ui;

    QTimer timer;

    ImgViewer *visorS, *visorD, *visorS2, *visorGroundTruth;
    Mat colorImage, grayImage, destColorImage, destGrayImage, groundTruthImage;
    bool winSelected, selectColorImage;
    Rect imageWindow;
    int idReg;
    int width;          //Anchura de la imagen original

    Mat corners;        //Mat de esquinas izquierda
    Mat cornersD;       //Mat de esquinas derecha
    Mat imgRegiones;
    Mat imgMask;
    Mat detected_edges;
    Mat canny_image;    //Mat de canny
    Mat fijos;
    Mat disparidad;
    Rect minRect;       //Minima ventana de los puntos modificados (añadidos a la region)
    Region r;


    std::vector<Region> listRegiones;   //Lista de regiones
    std::vector<punto> listDisparidades;
    std::vector<Point> vecinos;         //Lista de vecinos
    std::vector<punto> cornerList;      //Lista de esquinas
    std::vector<punto> cornerListD;      //Lista de esquinas
    std::vector<QLine> lineList;        //Lista de lineas
    std::vector<Point> pCorte;          //Lista de puntos validos
    std::vector<QLine> segmentList;     //Lista de segmentos

public slots:
    void compute();
    void change_color_gray(bool color);
    void selectWindow(QPointF p, int w, int h);
    void deselectWindow();
    void loadFromFile();
    void loadGroundTruth();
    void saveToFile();
    void initialize();
    void segmentation();
    void initVecinos();
    int vecinoMasSimilar(int x, int y);
    void vecinosFrontera();
    void bottomUp();
    void asignarBordesARegion();
    void mostrarListaRegiones();

    void cornerDetection();
    void initDisparity();
};


#endif // MAINWINDOW_H
