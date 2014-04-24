#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkLine.h>
#include <vtkCellArray.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

#include <vigra/hdf5impex.hxx>

std::pair< vtkSmartPointer<vtkPolyData>,
           vtkSmartPointer<vtkPolyData> >
readMesh(const std::string& filename) {
    using namespace vigra;
    HDF5File f(filename, HDF5File::OpenReadOnly);
    
    MultiArray<2, uint32_t> verts;
    MultiArray<2, uint32_t> faces;
    MultiArray<2, uint32_t> lines;
    
    f.readAndResize("verts", verts);
    f.readAndResize("faces", faces);
    f.readAndResize("lines", lines);
    
    std::cout << "verts = " << verts.shape() << std::endl;
    std::cout << "faces = " << faces.shape() << std::endl;
    std::cout << "lines = " << lines.shape() << std::endl;
    
    vtkSmartPointer<vtkPoints> pts=vtkSmartPointer<vtkPoints>::New();
    {
        float v[3];
        for(size_t i=0; i<verts.shape(0); ++i) {
            for(size_t j=0; j<3; ++j) { v[j] = verts(i,j); }
            pts->InsertNextPoint(v);
        }
    }
    
    vtkSmartPointer<vtkPolyData> linesPolyData=vtkSmartPointer<vtkPolyData>::New();
    {
        vtkSmartPointer<vtkCellArray> vtklines=vtkSmartPointer<vtkCellArray>::New();
        for(size_t i=0; i<lines.shape(0); ++i) {
            vtkSmartPointer<vtkLine> line0=vtkSmartPointer<vtkLine>::New();
            line0->GetPointIds()->SetId(0, lines(i,0));
            line0->GetPointIds()->SetId(1, lines(i,1));
            vtklines->InsertNextCell(line0);
        }
        linesPolyData->SetPoints(pts);
        linesPolyData->SetLines(vtklines);
    }
    
    vtkSmartPointer<vtkPolyData> facePolyData = vtkSmartPointer<vtkPolyData>::New();
    {
        vtkSmartPointer<vtkCellArray> polys = vtkSmartPointer<vtkCellArray>::New();
        vtkIdType pts4[4];
        for(size_t i=0; i<faces.shape(0); ++i) {
            for(size_t j=0; j<4; ++j) { pts4[j] = faces(i,j); }
            polys->InsertNextCell(4, pts4);
        }
        facePolyData->SetPoints(pts);
        facePolyData->SetPolys(polys);
    }
    
    return std::make_pair(linesPolyData, facePolyData);
}

int main(int argc, char **argv) {
    if(argc != 2) {
        std::cout << "usage: ./showmesh hdf5file" << std::endl;
        return 0;
    }
    std::string hdf5file(argv[1]);
   
    std::pair< vtkSmartPointer<vtkPolyData>,
               vtkSmartPointer<vtkPolyData> > d = readMesh(hdf5file);
    
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInput(d.second);

    vtkSmartPointer<vtkActor> actor= vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    vtkSmartPointer<vtkRenderer> renderer=vtkSmartPointer<vtkRenderer>::New();
    renderer->AddActor(actor);
    renderer->SetBackground(0,0,0);
    vtkSmartPointer<vtkRenderWindow> renderWindow=vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer(renderer);
    vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor=vtkSmartPointer<vtkRenderWindowInteractor>::New();
    renderWindowInteractor->SetRenderWindow(renderWindow);
    renderWindow->Render();
    renderWindowInteractor->Start();
}