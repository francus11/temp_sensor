using System;
using System.IO.Ports;
using System.Linq;
using System.Threading;
using System.Windows;
using System.Windows.Threading;

namespace terminal
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        static SerialPort serialPort;
        readonly int[] baudRates = { 300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 74880, 115200, 230400, 250000 };
        static private bool isConnected = false;
        string data;
        Thread readSerial;

        private DispatcherTimer timer;

        public MainWindow()
        {
            serialPort = new SerialPort();
            serialPort.ReadTimeout = 2200;
            serialPort.WriteTimeout = 2200;
            InitializeComponent();
            

            timer = new DispatcherTimer();
            timer.Interval = TimeSpan.FromSeconds(1); // Set the interval to 1 second
            timer.Tick += Timer_Tick;

            // Start the timer
            timer.Start();

        }

        private void Timer_Tick(object sender, EventArgs e)
        {
            RefreshAvailableDevices();
        }

        private void RefreshAvailableDevices()
        {
            deviceSelector.Items.Clear();
            foreach (object device in SerialPort.GetPortNames())
            {
                deviceSelector.Items.Add(device);
            }

            if (deviceSelector.Items.Count == 0)
            {
                deviceSelector.IsEnabled = false;
            }
            else if (deviceSelector.Items.Count == 1)
            {
                deviceSelector.IsEnabled = false;
                deviceSelector.SelectedIndex = 0;
            }
            else
            {
                deviceSelector.IsEnabled = true;
            }

            IsAvailableDevice();
        }

        private void deviceSelector_Loaded(object sender, RoutedEventArgs e)
        {
            RefreshAvailableDevices();
        }

        private void baudRateSelector_Loaded(object sender, RoutedEventArgs e)
        {
            foreach (int baudRate in baudRates)
            {
                baudRateSelector.Items.Add(baudRate);
            }

            //set default selection (9600)
            baudRateSelector.SelectedItem = baudRateSelector.Items[4];
        }

        private void InitializeSerial()
        {
            serialPort.PortName = (string)deviceSelector.SelectedValue;
            serialPort.BaudRate = (int)baudRateSelector.SelectedValue;
            serialPort.Parity = Parity.None;
            serialPort.DataBits = 8;
            serialPort.StopBits = StopBits.One;
            serialPort.Handshake = Handshake.RequestToSendXOnXOff;
        }

        private void ConnectButton_Click(object sender, RoutedEventArgs e)
        {
            if (!isConnected)
            {
                InitializeSerial();
                ConnectButton.Content = "Disconnect";

                isConnected = true;
                serialPort.Open();
                readSerial = new Thread(Read);
                readSerial.Name = "Search For serial data";
                readSerial.Start();


            }
            else
            {
                ConnectButton.Content = "Connect";

                isConnected = false;
                readSerial.Join();
                readSerial.Interrupt();
                serialPort.Close();
            }
        }

        private void RefreshButton_Click(object sender, RoutedEventArgs e)
        {
            RefreshAvailableDevices();
        }

        private void IsAvailableDevice()
        {
            if (SerialPort.GetPortNames().Any())
            {
                ConnectButton.IsEnabled = true;
            }
            else
            {
                ConnectButton.IsEnabled = false;
            }
        }

        private void Read()
        {
            while (isConnected)
            {
                string message;
                try
                {
                    message = serialPort.ReadLine();
                    
                }
                catch (TimeoutException e)
                {
                    message = e.Message;
                }
                try
                {
                    //there is a problem because when closing connection, main thread waits for Read thread to finish, but then thread waits to execute line below. Soooo, this is impossible
                    // Figure out new way to send data to main thread
                    // Maybe something with lock
                    Application.Current.Dispatcher.Invoke(() => texty.Text = message);
                }
                catch (Exception e)
                {
                    data = e.Message;
                }
                
                
            }
        }

        //TODO: Connecting to the Serial device
        //TODO: Refresh available Serial devices automatically (done)
        //TODO: Sending and receiving data to/from device
        //TODO: Redesign style and format of changing settings
    }
}
