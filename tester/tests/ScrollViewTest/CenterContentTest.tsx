import { TestSuite } from '@rnoh/testerino';
import { TestCase } from '../../components';
import { ScrollView, View, Text, StyleSheet } from 'react-native';

export function CenterContentTest() {
  return (
    <TestSuite name="centerContent">
      <TestCase.Example
        tags={['C_API']}
        modal
        itShould="Content is not centered (centerContent is false)"
      >
        <View style={styles.container}>
          <ScrollView
            style={styles.scrollView}
            centerContent={false}
          >
            <Text style={styles.text}>
              The default value is false, Content is not centered.
            </Text>
          </ScrollView>
        </View>
      </TestCase.Example>
      <TestCase.Example
        tags={['C_API']}
        modal
        itShould="Content is centered (centerContent is true and the content is smaller than the scroll view)"
      >
        <View style={styles.container}>
          <ScrollView
            style={styles.scrollView}
            centerContent={true}
          >
            <Text style={styles.text}>
              When the content is smaller than the scroll view and the value is true, content is centered.
            </Text>
          </ScrollView>
        </View>
      </TestCase.Example>
      <TestCase.Example
        tags={['C_API']}
        modal
        itShould="Content is not centered (centerContent is true and the content is larger than the scroll view)"
      >
        <View style={styles.container}>
          <ScrollView
            style={styles.scrollView}
            centerContent={true}
          >
            <Text style={styles.text}>
              When the content is larger than the scroll view, this property has no effect. The default value is false.
              When the content is larger than the scroll view, this property has no effect. The default value is false.
              When the content is larger than the scroll view, this property has no effect. The default value is false.
              When the content is larger than the scroll view, this property has no effect. The default value is false.
              When the content is larger than the scroll view, this property has no effect. The default value is false.
            </Text>
          </ScrollView>
        </View>
      </TestCase.Example>
    </TestSuite>
  );
}

const styles = StyleSheet.create({
  container: {
    height: 200,
    width: '100%'
  },
  scrollView: {
    borderWidth: 3,
    borderColor: 'firebrick',
    flex: 1
  },
  text: {
    fontSize: 20,
    margin: 10,
  },
});
