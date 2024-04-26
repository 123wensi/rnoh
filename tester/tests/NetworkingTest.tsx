import {StyleSheet, Text, View} from 'react-native';
import {TestSuite} from '@rnoh/testerino';
import React from 'react';
import {TestCase} from '../components';

const FILE_URI = '/data/storage/el2/base/files/testFile.txt';

const WebSocketEcho = () => {
  const [status, setStatus] = React.useState('Loading...');
  const [data, setData] = React.useState<string>();

  const runWebSockSession = () => {
    // connect to Postman's echo server
    var ws = new WebSocket('wss://ws.postman-echo.com/raw');

    ws.onopen = () => {
      setStatus('Connected');
      ws.send('something');
    };

    ws.onmessage = e => {
      setData(JSON.stringify(e));
      setTimeout(() => {
        setStatus('Closing...');
        ws.close();
      }, 3000);
    };

    ws.onerror = e => {
      console.error(e.message);
      setStatus(`Error ${e.message}`);
    };

    ws.onclose = e => {
      console.log(e.code, e.reason);
      setStatus(`Closed ${e.code} ${e.reason}`);
    };
  };

  React.useEffect(() => {
    runWebSockSession();
  }, []);

  return (
    <View>
      <Text style={styles.loadingText}>{status}</Text>
      {data && <Text style={styles.movieDetails}>{data}</Text>}
    </View>
  );
};

export const NetworkingTest = () => {
  const canFetch = async (url: string) => {
    try {
      const response = await fetch(url);
      await response.json();
      return true;
    } catch (error) {
      return false;
    }
  };

  return (
    <TestSuite name="Networking">
      <TestCase.Logical
        tags={['C_API']}
        itShould="download data"
        fn={async ({expect}) => {
          const response = await fetch('https://reactnative.dev/movies.json');
          const result = await response.text();
          const parsed = JSON.parse(result);
          expect(parsed.title).to.be.eq('The Basics - Networking');
        }}
      />
      <TestCase.Logical
        tags={['C_API']}
        itShould="download and parse JSON data"
        fn={async ({expect}) => {
          const response = await fetch('https://reactnative.dev/movies.json');
          const result = await response.json();
          expect(result.title).to.be.eq('The Basics - Networking');
        }}
      />
      <TestCase.Logical
        tags={['C_API']}
        itShould="download data to an ArrayBuffer"
        fn={async ({expect}) => {
          const response = await fetch('https://reactnative.dev/movies.json');
          const result = await response.arrayBuffer();
          expect(result.byteLength).to.be.eq(458);
        }}
      />
      <TestCase.Logical
        tags={['C_API']}
        itShould="download data to a blob"
        fn={async ({expect}) => {
          const response = await fetch('https://reactnative.dev/movies.json');
          const result = await response.blob();
          expect(result.size).to.be.eq(458);
        }}
      />
      <TestCase.Logical
        tags={['C_API']}
        itShould="correctly send requests with FormData"
        fn={async ({expect}) => {
          const formData = new FormData();
          formData.append('name', 'test');
          formData.append('surname', 'test2');
          formData.append('boolean', true);
          formData.append('number', 72);
          const response = await fetch('https://httpbin.org/post', {
            method: 'POST',
            body: formData,
          });
          const result = await response.json();
          expect(result.form.name).to.be.eq('test');
          expect(result.form.boolean).to.be.eq('true');
          expect(result.form.number).to.be.eq('72');
          expect(result.form.surname).to.be.eq('test2');
        }}
      />
      <TestCase.Logical
        tags={['C_API']}
        itShould="correctly send a text file in FormData"
        fn={async ({expect}) => {
          try {
            let formData = new FormData();
            formData.append('file', {
              uri: FILE_URI,
              type: 'text/plain',
              name: 'testFile.txt',
            });
            formData.append('text', 'test text field');
            let response = await fetch('https://httpbin.org/post', {
              method: 'POST',
              body: formData,
              headers: {
                'Content-Type': 'multipart/form-data',
              },
            });
            let responseJson = await response.json();
            expect(responseJson.form.text).to.be.eq('test text field');
            expect(responseJson.files.file).to.exist;
          } catch (error) {
            console.error(
              'To run this test, you need to create a file at the correct path on the phone. You can run this command in the hdc shell: echo "Test file content" >> /data/app/el2/100/base/com.rnoh.tester/files/testFile.txt',
            );
            throw error;
          }
        }}
      />
      <TestCase.Logical
        tags={['C_API']}
        itShould="correctly read response headers"
        fn={async ({expect}) => {
          const response = await fetch(
            'https://httpbin.org/response-headers?Set-Cookie=theme%3Dlight&Set-Cookie=sessionToken%3Dabc123',
          );
          const cookies = response.headers.get('set-cookie');
          expect(cookies).to.be.oneOf([
            'theme=light, sessionToken=abc123',
            'theme=light,sessionToken=abc123',
          ]);
        }}
      />
      <TestCase.Logical
        tags={['C_API']}
        itShould="fail on bad url"
        fn={async ({expect}) => {
          const received = await canFetch(
            'https://reactnative.dev/bad_url.json',
          );
          expect(received).to.be.false;
        }}
      />
      <TestCase.Example itShould="connect to websocks">
        <WebSocketEcho />
      </TestCase.Example>
    </TestSuite>
  );
};

const styles = StyleSheet.create({
  movieDetails: {
    height: 20,
    width: '100%',
  },
  loadingText: {
    fontSize: 20,
    height: 40,
    width: '100%',
  },
});
