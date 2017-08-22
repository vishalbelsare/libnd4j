//
// @author raver119@gmail.com
//

#ifndef PROJECT_CONVOLUTIONLAYER_H
#define PROJECT_CONVOLUTIONLAYER_H

#include <layers/layers.h>
#include <layers/generic/BaseLayer.h>

namespace nd4j {
namespace layers {

template<typename T, typename AF> class ConvolutionLayer: public BaseLayer<T, AF> {
    private:
        int _kernelH, _kernelW;             // kernel sizes
        int _strideH, _strideW;             // step of kernel slide across the width and height of the input volume/picture 
        int _padH, _padW;                   // the number of zero-columns and zero-rows at the edges of input volume/picture 
        bool _padModeSame;

    public:
        // default constructor 
        ConvolutionLayer() = delete;

        // constructor 
        ConvolutionLayer(const int kernelH, const int kernelW, const int strideH, const int strideW, const int padH, const int padW, const bool padModeSame);
        
        // This method should validate input parameters, and return TRUE if everything ok. FALSE otherwise
        virtual int validateInput() const;

        // This method should validate output parameters, and return TRUE if everything is ok, FALSE otherwise        
        virtual int validateOutput() const;

        // This method "allocates" memory chunk from workspace
        // virtual T* allocate(long bytes) = 0; 
        
        // This method should validate parameters & bias, and return TRUE if everything ok. False otherwise
        virtual int validateParameters() const;

        // this method should validate memory/holders for BP pass
        virtual int validateGradients() const;

        // feed forward
        virtual int feedForward();

        // back propagate
        virtual int backPropagate();
        
};



//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
///////////////////// implementation part ////////////////////////////

// constructor 
template<typename T, typename AF> ConvolutionLayer<T,AF>::ConvolutionLayer(const int kernelH, const int kernelW, const int strideH, const int strideW, const int padH, const int padW, const bool padModeSame): BaseLayer<T,AF>()  {
    _kernelH     = kernelH;  
    _kernelW     = kernelW;
    _strideH     = strideH;
    _strideW     = strideW;    
    _padH        = padH;     
    _padW        = padW; 
    _padModeSame = padModeSame;
}

//////////////////////////////////////////////////////////////////////
// This method should validate input parameters, and return TRUE if everything ok. FALSE otherwise
template<typename T, typename AF> int ConvolutionLayer<T,AF>::validateInput() const {
    
   if (this->_input == nullptr || !this->_input->nonNull())        
        return ND4J_STATUS_BAD_INPUT;

    if (this->_input->rankOf() != 4)
        return ND4J_STATUS_BAD_RANK;

    if (this->_input->getShapeInfo()[2] != this->_params->getShapeInfo()[2])
        return ND4J_STATUS_BAD_SHAPE;        
    
}

//////////////////////////////////////////////////////////////////////
// This method should valudate output parameters, and return TRUE if everything is ok, FALSE otherwise
template<typename T, typename AF> int ConvolutionLayer<T,AF>::validateOutput() const {
    
    if (this->_output == nullptr || !this->_output->nonNull())
        return ND4J_STATUS_BAD_OUTPUT;

    if (this->_output->rankOf() != 4)
        return ND4J_STATUS_BAD_RANK;

    if (this->_output->getShapeInfo()[1] != this->_input->getShapeInfo()[1] || this->_output->getShapeInfo()[2] != this->_params->getShapeInfo()[1])
        return ND4J_STATUS_BAD_OUTPUT;
    
    float oH = (this->_input->getShapeInfo()[3] - _kernelH + 2.*_padH) / (_strideH + 1.);
    // oH must be integer ! 
    if (oH != (int)oH)
        return ND4J_STATUS_BAD_SHAPE;
    float oW = (this->_input->getShapeInfo()[4] - _kernelW + 2.*_padW) / (_strideW + 1.);
    // oH must be integer ! 
    if (oW != (int)oW)
        return ND4J_STATUS_BAD_SHAPE;
        
    return ND4J_STATUS_OK;
}

//////////////////////////////////////////////////////////////////////
template<typename T, typename AF> int ConvolutionLayer<T,AF>::validateParameters() const {
    
    if (this->_params == nullptr || !this->_params->nonNull() || this->_bias == nullptr || !this->_bias->nonNull())
        return ND4J_STATUS_BAD_PARAMS;    
    
    if (this->_params->rankOf() != 4 || this->_bias->rankOf() != 2)
        return ND4J_STATUS_BAD_RANK;

    // check the weight matrix consistency with class kernel members 
    if (this->_params->getShapeInfo()[3] != _kernelH || this->_params->getShapeInfo()[4] != _kernelW)
        return ND4J_STATUS_BAD_PARAMS;

    if (this->_params->getShapeInfo()[1] != this->_bias->getShapeInfo()[2])
        return ND4J_STATUS_BAD_SHAPE;
    
    return ND4J_STATUS_OK;
}


//////////////////////////////////////////////////////////////////////
template<typename T, typename AF> int ConvolutionLayer<T,AF>::validateGradients() const {

    // if (this->_gradientW == nullptr || this->_gradientB == nullptr ||  !this->_gradientW->nonNull() || !this->_gradientB->nonNull())
        // return ND4J_STATUS_BAD_GRADIENTS;    

    // if (this->_epsilonNext == nullptr || !this->_epsilonNext->nonNull())
        // return ND4J_STATUS_BAD_OUTPUT;
        
    // if (!this->_gradientW->isSameShape(*this->_params)) 
        // return ND4J_STATUS_BAD_GRADIENTS;
    
    // if (!this->_gradientB->isSameShape(*this->_bias))
        // return ND4J_STATUS_BAD_BIAS;

    // // we're checking equality of input/epsilon batch size
    // if (this->_epsilon->shapeOf()[0] != this->_input->shapeOf()[0])
        // return ND4J_STATUS_BAD_EPSILON;

    // if (this->_epsilon->columns() != this->_bias->columns())
        // return ND4J_STATUS_BAD_EPSILON;

    // // batch comparison again
    // if (!this->_epsilonNext->isSameShape(*this->_input))
        // return ND4J_STATUS_BAD_OUTPUT;

    // return ND4J_STATUS_OK;
}

//////////////////////////////////////////////////////////////////////
// feed forward
template<typename T, typename AF> int ConvolutionLayer<T,AF>::feedForward( ) {
   
    functions::transform::exec<simdOps::Im2col<T>>(_input->getBuff(), _input->getShapeInfo(), _output->getBuff(), _output->getShapeInfo())
    
    // permute and reshape weights (_params) to 2D
    if (!_params->permute({3, 2, 1, 0})) 
        return ND4J_STATUS_BAD_PARAMS;    
    if(_params.reshape(_kernelW*_kernelH*_input->getShapeInfo()[2], _output->getShapeInfo()[2]))
        return ND4J_STATUS_BAD_PARAMS;

    gemmHelper(_input, _params, _output, (T) 1.0f, (T) 0.0f);
    
    _output->addiRowVector(this->_bias);    

    ActivationsExecutioner<T>::template executeFF<AF>(_output, _output);

    // permute and reshape weights (_params) back to 4D
    _params.reshape(_kernelW, _kernelH, _input->getShapeInfo()[2], _output->getShapeInfo()[2]);
    _params->permute({3, 2, 1, 0});

    return ND4J_STATUS_OK;
}

// back propagation
template<typename T, typename AF> int ConvolutionLayer<T,AF>::backPropagate() {

} 

// end of namespace brackets
}
}


#endif //PROJECT_CONVOLUTIONLAYER_H