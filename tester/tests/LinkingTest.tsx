import {Linking} from 'react-native';
import {TestSuite} from '@rnoh/testerino';
import {Button, TestCase} from '../components';

export function LinkingTest() {
  return (
    <TestSuite name="Linking (Stub)">
      <TestCase.Logical
        tags={['C_API']}
        itShould="not crash when checking if url can be opened"
        fn={async ({expect}) => {
          expect(await Linking.canOpenURL('http://foobar.com')).to.be.true;
        }}
      />
      <TestCase.Logical
        tags={['C_API']}
        itShould="support http/https urls"
        fn={async ({expect}) => {
          expect(await Linking.canOpenURL('http://foobar.com')).to.be.true;
          expect(await Linking.canOpenURL('https://foobar.com')).to.be.true;
        }}
      />
      <TestCase.Logical
        tags={['C_API']}
        itShould="support tel urls"
        fn={async ({expect}) => {
          expect(await Linking.canOpenURL('tel:1234567890')).to.be.true;
        }}
      />
      <TestCase.Logical
        tags={['C_API']}
        itShould="support sms urls"
        fn={async ({expect}) => {
          expect(await Linking.canOpenURL('sms:1234567890')).to.be.true;
        }}
      />
      <TestCase.Logical
        tags={['C_API']}
        itShould="support map url"
        fn={async ({expect}) => {
          expect(await Linking.canOpenURL('maps://navigation')).to.be.true;
        }}
      />
      <TestCase.Logical
        tags={['C_API']}
        itShould="not support wrong schema urls"
        fn={async ({expect}) => {
          expect(await Linking.canOpenURL('wrong://host')).to.be.false;
        }}
      />
      <TestCase.Example tags={['C_API']} itShould="open phone dialing on press">
        <Button
          onPress={() => Linking.openURL('tel:1234567890')}
          label="Call 1234567890"
        />
      </TestCase.Example>
      <TestCase.Example
        tags={['C_API']}
        itShould="open sms app (sms app doesn't accept parameters)">
        <Button
          onPress={() => Linking.openURL('sms:1234567890')}
          label="Send SMS to 1234567890"
        />
      </TestCase.Example>
      <TestCase.Example tags={['C_API']} itShould="open map app on press">
        <Button
          onPress={() => Linking.openURL('maps://navigation')}
          label="Open Map app"
        />
      </TestCase.Example>
      <TestCase.Example tags={['C_API']} itShould="open web page on press">
        <Button
          onPress={() =>
            Linking.openURL('https://reactnative.dev/docs/linking')
          }
          label="Open Linking Docs"
        />
      </TestCase.Example>
      <TestCase.Example
        tags={['C_API']}
        itShould="open (http) web page on press">
        <Button
          onPress={() => Linking.openURL('http://info.cern.ch/')}
          label="Open an old webpage"
        />
      </TestCase.Example>
      <TestCase.Example tags={['C_API']} itShould="open application settings">
        <Button onPress={() => Linking.openSettings()} label="Open settings" />
      </TestCase.Example>
      <TestCase.Example tags={['C_API']} itShould="fail on bad url">
        <Button
          onPress={() =>
            Linking.openURL('bad://url').catch(e => console.warn(e))
          }
          label="Bad URL"
        />
      </TestCase.Example>
    </TestSuite>
  );
}
