import React, {useRef, useState} from 'react';
import {Text, View, VirtualizedList} from 'react-native';
import {TestCase, TestSuite} from '@rnoh/testerino';
import {Button} from '../components';

type OnScrollToIndexFailed = {
  index: number;
  highestMeasuredFrameIndex: number;
  averageItemLength: number;
};

type ItemData = {
  id: string;
  title: string;
};

const getItem = (_data: unknown, index: number): ItemData => ({
  id: index.toString(),
  title: `Item ${index}`,
});

const getItemCountVirtualized = (_data: unknown): number => 50;

const Item = ({title}: {title: string}) => (
  <View style={{height: 48, padding: 16}}>
    <Text style={{width: '100%', height: 24}}>{title}</Text>
  </View>
);

export function VirtualizedListTest() {
  return (
    <TestSuite name="VirtualizedList">
      <TestCase itShould="display list of 3 items">
        <VirtualizedList<number[]>
          style={{height: 64}}
          data={[1, 2, 3]}
          getItem={(data, idx) => data[idx]}
          getItemCount={() => 3}
          renderItem={({item}) => (
            <View style={{height: 48, padding: 16}}>
              <Text style={{width: '100%', height: 24}}>{item}</Text>
            </View>
          )}
          keyExtractor={(_, index) => index}
        />
      </TestCase>
      <TestCase
        modal
        itShould="trigger onStartReached event when start of the content is within half the visible length of the list"
        initialState={-1}
        arrange={({setState}) => {
          const data = [1, 2, 3, 4, 5];
          return (
            <VirtualizedList
              data={data}
              getItem={(data: number[], idx: number) => data[idx]}
              getItemCount={() => data.length}
              renderItem={({item}: {item: number}) => (
                <View style={{height: 100, padding: 16, borderWidth: 1}}>
                  <Text style={{width: '100%', height: 24}}>{item}</Text>
                </View>
              )}
              style={{height: 200}}
              onStartReachedThreshold={0.5}
              onStartReached={({
                distanceFromStart,
              }: {
                distanceFromStart: number;
              }) => {
                setState(distanceFromStart);
              }}
            />
          );
        }}
        assert={({state, expect}) => {
          expect(state).to.be.lessThanOrEqual(100);
        }}
      />
      <TestCase
        modal
        itShould="display event sent to by onScrollToIndexFailed when pressing the button before scrolling"
        initialState={undefined}
        arrange={({state, setState}) => {
          const ref = useRef<VirtualizedList<ItemData>>(null);

          const handleOnPress = () => {
            if (ref.current) {
              ref.current.scrollToIndex({index: 20, animated: true});
            }
          };

          return (
            <>
              <Button
                label="Scroll to NOT_EXISTING index"
                onPress={handleOnPress}
              />
              <View style={{height: 50, backgroundColor: 'lightblue'}}>
                <Text>{state ? JSON.stringify(state) : ''}</Text>
              </View>
              <VirtualizedList<number[]>
                initialNumToRender={5}
                windowSize={5}
                ref={ref}
                style={{height: 128}}
                getItem={getItem}
                getItemCount={getItemCountVirtualized}
                renderItem={({item}: {item: ItemData}) => (
                  <Item title={item.title} />
                )}
                keyExtractor={(item: ItemData) => item.id}
                onScrollToIndexFailed={(failInfo: OnScrollToIndexFailed) => {
                  // @ts-ignore
                  setState(failInfo);
                }}
              />
            </>
          );
        }}
        assert={({state, expect}) => {
          expect(state).to.be.not.undefined;
          expect(state).to.have.all.keys([
            'index',
            'highestMeasuredFrameIndex',
            'averageItemLength',
          ]);
        }}
      />
    </TestSuite>
  );
}
