function generateRandomMatrix(rows, cols, min, max) {
  const matrix = [];
  
  for (let i = 0; i < rows; i++) {
      const row = [];
      for (let j = 0; j < cols; j++) {
          const randomNum = Math.floor(Math.random() * (max - min + 1)) + min;
          row.push(randomNum);
      }
      matrix.push(row);
  }
  
  return matrix;
}


const matrix = generateRandomMatrix(1, 1, -10, 10);
console.log(matrix.map((array) => array.join(' ')).join('\n') );