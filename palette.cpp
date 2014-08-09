#include <list>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;

#include "CImg-1.5.7/CImg.h"
using namespace cimg_library;

#include "lib/ColorHist.h"
#include "lib/Color.h"
#include "lib/Triplet.h"


/** 
    This program determine the best color palette to use to display an
    image, using the K-mean algorithm.
    Use : 'palette %filepath %colorsCount'
    Parameters :
    filepath : The path to the image file.
    colorsCount : The desired number of colors in the palette used
    Result :
    An image containing the palette's colors will be saved.
*/


/** This struct represent a point in the RGB space, along with the
    cluster it currently belongs to. */
struct point {
    // The RGB coordinates
    Color color;
    // The cluster index this point belongs to
    unsigned int cluster;
};


inline Color getPixel(int const x, int const y, CImg<unsigned char> const& image) {
    return Color(image(x, y, 0), image(x, y, 1), image(x, y, 2));
}



void generatePalette(char * file, int K) {

    // Load the image
    CImg<unsigned char> image(file);
    // Create a display for the original image
    CImgDisplay orig_disp(image, "Original");
  
    // The list of the uniques colors from the image. It corresponds to
    // the points (in RGB space) for the K-mean algorithm.
    list<point> pointList;
    // The number of occurrence of each color from the RGB space in the
    // input image.
    ColorHist ch;
    // Temporary variable
    Color c;
    // Temporary variable
    point p;
 

    // Indicates if the kmeans have changed during the last step. False
    // means that the algorithm has converged.
    bool changed = true;
    // Index of the nearest color
    int nearest;
    // The distance from the nearest color
    double nearestDist;
    // The temporary calculated distance
    double dist;
    // The number of point associated with each cluster
    long * clusterWeight = new long[K];
    // Temporary weight
    long weight;

    // The centers of the clusters in the k-mean algorithm. At the end :
    // the computed color palette
    Color * kmean;
    // The next computed kmeans. Acts as an intermediate buffer 
    Triplet * nextKMean = new Triplet[K];

    // Initialize the means array
    kmean = new Color[K];    

    // Initialize the color histogram and the point list

    for (int i = 0 ; i < image.height() ; i++) {
	for (int j = 0 ; j < image.width() ; j++) { // For each pixel
	    // 
	    p.color = getPixel(j, i, image);
	    p.cluster = 0; 
	    // Add the color to the histogram
	    if (!ch.addColor(p.color))
		// If the color didn't exist already, add it to the list of all the differents colors
		pointList.push_front(p);
	}
    }

    // Check that there are enough different colors in the original image
    if (pointList.size() < (unsigned int) K) { 
	// Not enough colors
	cerr << "The original image has less colors than the desired number of colors in the palette." << endl;
	exit(1);
    }

    // Initialize the clusters centers (kmeans) with colors from the image
    {
	// Select K colors evenly spaced in the list of all unique colors
	list<point>::iterator iterator = pointList.begin();
	unsigned int step = pointList.size()/(unsigned int) K;
	for (int i = 0; i < K; i++) {
	    // Initialize the kmean with this color
	    c = (*iterator).color;
	    kmean[i] = c;
	    // Next step
	    std::advance(iterator, step);
	}
    }
  
    ////////////////// K-Mean Algorithm ////////////////////////


    // While the algorithm modifies the kmeans
    while(changed) {
	// Initialize the next k-means array
	for (int i = 0 ; i < K ; i++ ) {
	    nextKMean[i] = Triplet(0,0,0);
	    clusterWeight[i] = 0;
	}
     
	// Update the clusters by re-associating the points to the closest kmean
	for (list<point>::iterator iterator = pointList.begin() ; 
	     iterator != pointList.end() ; iterator++) {
	    // Get the color being treated
	    c = (*iterator).color;
       
	    // Initialize the nearest kmean as the first of the list
	    nearest = 0;
	    // Initialize with the maximum distance
	    nearestDist = 4096; // sqrt(256*256*256)
	    // Find the closest kmean
	    for (int i = 0 ; i < K ; i++ ) {
		// Distance from the i-th kmean
		dist = Color::distance(c, kmean[i]);
		// If the distance is best
		if (dist < nearestDist) {
		    // Update nearest variables
		    nearestDist = dist;
		    nearest = i;
		}
	    }
	    // This color belongs to the cluster of the nearest kmean
	    (*iterator).cluster = nearest;
	
	    // Account the number of pixel of this color
	    weight = ch.getColor(c);
	    // Convert to a triplet
	    Triplet triplet = c.toTriplet();
	    // Multiply by the weight
	    triplet.multiply(weight);
	    // Add it to the total
	    nextKMean[nearest].add(triplet);

	    // Add to the total weight of the nearest kmean
	    clusterWeight[nearest] += weight;
	
	}
	// All color pixels have been re-assigned to their nearest cluster


	// Divide the sums of color of each kmean by their weight. The
	// calculated barycenter is the new kmean. Finally we check
	// whether this has changed the kmeans.
      
	changed = false;
	// For each kmean
	for (int i = 0 ; i < K ; i++ ) {
	    // If the cluster is not empty
	    if(clusterWeight[i] != 0) 
		// Calculate the barycenter
		nextKMean[i].divide(clusterWeight[i]);
	    // Convert as color
	    c = nextKMean[i].getColor();
	    // Update the changed boolean
	    changed = changed or
		(c.getR() != kmean[i].getR()) or
		(c.getG() != kmean[i].getG()) or
		(c.getB() != kmean[i].getB());
	    // Update the kmean
	    kmean[i] = c;
	}
	cout << ".";
	cout.flush();
    }

      
    // The algorithm has converged. The kmean represent our colors
    cout << "algorithm converged " << endl;
    // Determine the palette image size (at most 256 pixels wide)
    int height = K/256 + 1;
    int width = K%256;
    // Create the image that will contain the palette colors. 
    CImg<unsigned char> palette(width, height, 1, 3, 0);

    // Sort the palette color TODO

    // Generate the palette image
    for (int n = 0 ; n < K ; n++ ) {
	// Get the palette color
	c = kmean[n];
	// Create a pixel of the color c in the palette image
	// Line
	int i = n/palette.width();
	// Column
	int j = n%palette.width();
	// Fill the RGB channels
	palette(j, i, 0) = c.getR();
	palette(j, i, 1) = c.getG();
	palette(j, i, 2) = c.getB();
    }

    cout << "image filled " << endl;

    // Save the image (palette-K-filename)
    stringstream ss;
    ss << K;
    string string_palette = "palette-" + ss.str() + "-" + std::string(file);
    const char * char_palette = string_palette.c_str();
    palette.save(char_palette);

    cout << endl << "Palette image saved under " << char_palette << endl;
  
    delete[] nextKMean; // Warning, allows array overhead...
    delete[] kmean;
}


int main(int argc, char* argv[]) {

    // The file path
    char * file;
    // The number of colors in the palette. This correspond to the K of
    // the K-mean algorithm.
    int K;

    // If not enough arguments were given when called
    if (argc != 3) {
	// Display an error and exit
	cerr << "Expected : " << argv[0] << " %filepath %colorsCount" << endl;
	exit(1);
    }
  
    // Retrieve the file path argument
    file = argv[1];
    // Retrieve the number of colors argument
    K = atoi(argv[2]);

    // Check that K is between 2 and 65536
    if(K < 2 || K > 65536) {
	// Error and exit
	cerr << "Color Count must be in [2;65536]" << endl;
	exit(1);
    }

    generatePalette(file, K);

    return 0;
}