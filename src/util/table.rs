pub struct Table<T> {
    width: usize,
    height: usize,
    data: Vec<T>
}

impl<T: Default + Clone> Table<T> {
    pub fn new(width: usize, height: usize) -> Table<T> {
        let data = vec![T::default(); width * height];
        return Table { width, height, data };
    }

    pub fn at(&self, x: usize, y: usize) -> &T {
        return &self.data[y * self.width + x];
    }

    pub fn at_mut(&mut self, x: usize, y: usize) -> &mut T {
        return &mut self.data[y * self.width + x];
    }
}