def generate_dataset(array, p_in=30.0, p_s=60.0, sparsity=0.20):
    import numpy as np

    arrayShape = array.shape
    x = np.fft.fftfreq(arrayShape[0])
    y = np.fft.fftfreq(arrayShape[1])
    z = np.fft.fftfreq(arrayShape[2])

    X, Y, Z = np.meshgrid(y, x, z)
    kr = np.sqrt(X**2 + Y**2 + Z**2)

    # Create inner structure.
    A = np.exp(-p_in * kr) # Generate amplitude
    phase = np.random.randn(arrayShape[0], arrayShape[
                            1], arrayShape[2]) # Generate phase
    F = A * np.exp(2 * np.pi * 1j * phase) # Combine amplitude and phase
    f = np.fft.ifftn(F) # Inverse FFT
    f_in = np.absolute(f).copy().flatten()

    # Create shape
    A = np.exp(-p_s * kr) # Generate amplitude
    phase = np.random.randn(arrayShape[0], arrayShape[
                            1], arrayShape[2]) # Generate phase
    F = A * np.exp(2 * np.pi * 1j * phase) # Combine amplitude and phase
    f = np.fft.ifftn(F) # Inverse FFT
    f_shape = np.absolute(f)

    # Impose sparsity (% of non-zero voxels)
    f_shape = np.argsort(f_shape, axis=None) # Sort the shape image
    f_shape = f_shape.flatten()
    # Number of zero voxels
    N_zero = np.int(np.round((array.size * (1 - sparsity))))
    f_shape[N_zero:] = f_shape[N_zero]
    f_in[f_shape] = 0

    f_in = f_in / np.amax(f_in) # Normalize image
    np.copyto(array, f_in.reshape(arrayShape)) # Update array
