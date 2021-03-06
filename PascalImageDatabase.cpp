#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

#include "PascalAnnotation.h"
#include "PascalImageDatabase.h"

using namespace std;
using namespace cv;

//Pascal Image Database class

PascalImageDatabase::PascalImageDatabase():
    _positivesCount(0), _negativesCount(0)
{
}

PascalImageDatabase::PascalImageDatabase(const char *dbFilename, string category):
    _positivesCount(0), _negativesCount(0)
{
    _category = category;
    load(dbFilename);
}

PascalImageDatabase::PascalImageDatabase(const vector<float> &labels, const vector<string> &filenames):
    _positivesCount(0), _negativesCount(0)
{
    _labels = labels;
    _filenames = filenames;

    for(vector<float>::iterator i = _labels.begin(); i != _labels.end(); i++) {
        if(*i > 0) _positivesCount++;
        else if(*i < 0) _negativesCount++;
    }
}

bool PascalImageDatabase::getROI(string imageName, vector<Rect>& rois, vector<float>& roiLabels)
{
    vector<string> parts;
    boost::split(parts,imageName,boost::is_any_of("/."),boost::token_compress_on);

    string annotationsPath = "/Users/david/Documents/Development/VOC2007/VOCdevkit/VOC2007/Annotations/";
    string annotationsFilename = annotationsPath + parts[parts.size()-2] + ".xml";

    pascal_annotation annotation;
    annotation.load(annotationsFilename);

    //Mat img = imread(imageName,1);
    //cout << "Obtaining annotations from: " << imageName << endl;

    for(int i = 0; i < annotation.objects.size(); ++i){
        // if(annotation.objects[i].truncated == 1)
        //     return false;
        //cout << "Obtaining annotations for: " << annotation.objects[i].name << endl;

        int label = -1;
        if(boost::iequals(annotation.objects[i].name,_category))
            label = 1;

        Rect originalR;

        originalR.x = annotation.objects[i].bndbox.xmin;
        originalR.y = annotation.objects[i].bndbox.ymin;
        originalR.width = annotation.objects[i].bndbox.xmax - annotation.objects[i].bndbox.xmin;
        originalR.height = annotation.objects[i].bndbox.ymax - annotation.objects[i].bndbox.ymin;
        // rectangle(img,originalR,Scalar(255,0,0),2);
        // cout << originalR << endl;

        Rect r;

        r.x = annotation.objects[i].bndbox.xmin;
        r.y = annotation.objects[i].bndbox.ymin;
        r.width = annotation.objects[i].bndbox.xmax - annotation.objects[i].bndbox.xmin;
        r.height = annotation.objects[i].bndbox.ymax - annotation.objects[i].bndbox.ymin;

        // if(label == 1){
        //     int deltax = annotation.objects[i].bndbox.xmax - annotation.objects[i].bndbox.xmin;
        //     int deltay = annotation.objects[i].bndbox.ymax - annotation.objects[i].bndbox.ymin;
        //     if(deltax <= 64 || deltay <= 128){

        //         r.x = annotation.objects[i].bndbox.xmin;
        //         r.y = annotation.objects[i].bndbox.ymin;
        //         r.width = annotation.objects[i].bndbox.xmax - annotation.objects[i].bndbox.xmin;
        //         r.height = annotation.objects[i].bndbox.ymax - annotation.objects[i].bndbox.ymin;
        //         //cout << "Small image " << annotation.objects[i].bndbox.xmax << "-" << annotation.objects[i].bndbox.ymax << endl;
        //     }
        //     else
        //     {

        //         Point center(((annotation.objects[i].bndbox.xmax - annotation.objects[i].bndbox.xmin)/2)+annotation.objects[i].bndbox.xmin,
        //                         ((annotation.objects[i].bndbox.ymax - annotation.objects[i].bndbox.ymin)/2)+annotation.objects[i].bndbox.ymin);
        //         //cout << "Extracting the center: "<< center << endl;
        //         r.x = center.x-32;
        //         r.y = center.y-64;
        //         r.width = 64;
        //         r.height = 128;
        //     }
        //     //rectangle(img,r,Scalar(0,0,0),2);
        // }
        // else
        // {
        //     r.x = annotation.objects[i].bndbox.xmin;
        //     r.y = annotation.objects[i].bndbox.ymin;
        //     r.width = annotation.objects[i].bndbox.xmax - annotation.objects[i].bndbox.xmin;
        //     r.height = annotation.objects[i].bndbox.ymax - annotation.objects[i].bndbox.ymin;
        // }
            
        
        // imshow("GT",img);
        // waitKey(0);

        rois.push_back(r);
        roiLabels.push_back(label);
        //cout << "Clean exit " << r << endl;
    }
    return true;
}

void PascalImageDatabase::load(const char *dbFilename)
{
    string imagePath = "/Users/david/Documents/Development/VOC2007/VOCdevkit/VOC2007/JPEGImages/";

    _dbFilename = string(dbFilename);

    _negativesCount = 0;
    _positivesCount = 0;

    ifstream f(dbFilename);
    LOG(INFO) << "Loading the database";
    if(!f.is_open()) {
        throw std::runtime_error("ERROR: Could not open file " + _dbFilename + " for reading");
    }
    else{
        string line;
        int i = 0;
        while(getline(f,line)){
            if(i > 10)
                break;
            vector<string> parts;
            boost::split(parts,line,boost::is_any_of(" "),boost::token_compress_on);

            float label = atof(parts[1].c_str());
            string imageName = imagePath + parts[0] + ".jpg";

            vector<Rect> roi;
            vector<float> roiLabels;
            if(getROI(imageName, roi, roiLabels) == true)
            {
                Mat img = imread(imageName, CV_LOAD_IMAGE_GRAYSCALE);

                if(label > 0)
                {
                    for(int i = 0; i < roi.size(); ++i) 
                    {
                        //Mat roiImg = img(roi[i]);
                        _filenames.push_back(imageName);
                        _labels.push_back(roiLabels[i]);
                        _rois.push_back(roi[i]);
                        _flipped.push_back(false);

                        if(roiLabels[i] > 0)
                        {
                            // Add a flipped image
                            _filenames.push_back(imageName);
                            _labels.push_back(roiLabels[i]);
                            _rois.push_back(roi[i]);
                            _flipped.push_back(true);

                            _positivesCount++;
                        }  

                        if(roiLabels[i] < 0) _negativesCount++;
                        else if(roiLabels[i] > 0) _positivesCount++;
                    }
                }
                if(label < 0)
                {
                    //Get the random negatives
                    for(int i = 0; i < 10; i++)
                    {
                        _filenames.push_back(imageName);
                        _labels.push_back(-1);

                        if(img.cols <= 64 || img.rows <= 128){
                            _rois.push_back(roi[i]);
                            _flipped.push_back(false);
                            _negativesCount++;
                            i = 10;
                        }
                        else
                        {
                            int x = rand() % (img.cols-64);
                            int y = rand() % (img.rows-128);
                            Rect r(x,y,64,128);

                            _rois.push_back(r);
                            _flipped.push_back(false);

                            _negativesCount++;

                        }
                        
                    }
                }

                img.release();
                //i++;
            }
        }
    }
    f.close();
}

void PascalImageDatabase::save(const char *dbFilename)
{
    ofstream f(dbFilename);
    if(!f.is_open()) {
        throw std::runtime_error("ERROR: Could not open file " + (string)dbFilename + " for writing");
    }

    for(int i = 0; i < _labels.size(); i++) {
        f << _labels[i] << " " << _filenames[i] << "\n";
    }
}

ostream & operator << (ostream &s, const PascalImageDatabase &db)
{
    s << "DATABASE INFO\n"
      << setw(20) << "Original filename:" << " " << db.getDatabaseFilename() << "\n"
      << setw(20) << "Positives:" << setw(5) << right << db.getPositivesCount() << "\n"
      << setw(20) << "Negatives:"   << setw(5) << right << db.getNegativesCount() << "\n"
      << setw(20) << "Unlabeled:"  << setw(5) << right << db.getUnlabeledCount() << "\n"
      << setw(20) << "Total:"      << setw(5) << right << db.getSize() << "\n";

    return s;
}