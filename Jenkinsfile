pipeline {
  agent any
  stages {
    stage('build wamr') {
      agent any
      steps {
        sh '''cd product-mini/platforms/linux
mkdir build && cd build
cmake ..
make
'''
      }
    }

  }
}